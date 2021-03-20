#include "space.h"
#include "thread.h"
#include <assert.h>

#define TREE_CELL_LEAF_DIM 100


typedef struct TreeCell {
    struct TreeCell *lo, *hi;       /* child partitions */
    struct TreeCell *thread_next;
    TayAgentTag *first;             /* agents contained in this cell */
    unsigned count;                 /* agent counts for this cell */
    int dim;                        /* dimension along which the cell's partition is divided into child partitions */
    Box box;
    float mid;
} TreeCell;

typedef struct {
    CpuTree *tree;
    TayPass *pass;
    TreeCell *first_cell;
    unsigned agents_count;
} SeeTask;

static void _init_tree_see_task(SeeTask *task, CpuTree *tree, TayPass *pass, int thread_index) {
    task->tree = tree;
    task->pass = pass;
}

static void _thread_traverse_seen(CpuTree *tree, TreeCell *seer_cell, TreeCell *seen_cell, TayPass *pass, Box seer_box, TayThreadContext *thread_context) {
    for (int i = 0; i < tree->dims; ++i)
        if (seer_box.min.arr[i] > seen_cell->box.max.arr[i] || seer_box.max.arr[i] < seen_cell->box.min.arr[i])
            return;
    if (seen_cell->first) /* if there are any "seen" agents */
        pass->pairing_func(seer_cell->first, seen_cell->first, pass->see, pass->radii, tree->dims, thread_context);
    if (seen_cell->lo)
        _thread_traverse_seen(tree, seer_cell, seen_cell->lo, pass, seer_box, thread_context);
    if (seen_cell->hi)
        _thread_traverse_seen(tree, seer_cell, seen_cell->hi, pass, seer_box, thread_context);
}

static void _see_func(SeeTask *task, TayThreadContext *thread_context) {
    for (TreeCell *cell = task->first_cell; cell; cell = cell->thread_next) { /* seer cells */
        if (cell->first) {
            Box seer_box = cell->box;
            for (int i = 0; i < task->tree->dims; ++i) {
                seer_box.min.arr[i] -= task->pass->radii.arr[i];
                seer_box.max.arr[i] += task->pass->radii.arr[i];
            }
            _thread_traverse_seen(task->tree, cell, task->tree->cells, task->pass, seer_box, thread_context);
        }
    }
}

void cpu_tree_see(TayPass *pass) {
    static SeeTask tasks[TAY_MAX_THREADS];
    static SeeTask *sorted_tasks[TAY_MAX_THREADS];

    if (pass->seer_space == pass->seen_space) {
        Space *space = pass->seer_space;
        CpuTree *tree = &space->cpu_tree;

        /* reset tasks */
        for (int i = 0; i < runner.count; ++i) {
            tasks[i].first_cell = 0;
            tasks[i].agents_count = 0;
            sorted_tasks[i] = tasks + i;
        }

        TreeCell *buckets[TAY_MAX_BUCKETS] = { 0 };

        /* sort cells into buckets wrt number of contained agents */
        for (int i = 0; i < tree->cells_count; ++i) {
            TreeCell *cell = tree->cells + i;
            unsigned count = cell->count;
            if (count) {
                int bucket_i = space_agent_count_to_bucket_index(count);
                cell->thread_next = buckets[bucket_i];
                buckets[bucket_i] = cell;
            }
        }

        /* distribute cells among threads */
        for (int bucket_i = 0; bucket_i < TAY_MAX_BUCKETS; ++bucket_i) {
            TreeCell *cell = buckets[bucket_i];

            while (cell) {
                TreeCell *next_cell = cell->thread_next;

                SeeTask *task = sorted_tasks[0]; /* always take the task with fewest agents */
                cell->thread_next = task->first_cell;
                task->first_cell = cell;
                task->agents_count += cell->count;

                /* sort the task wrt its number of agents */
                {
                    int index = 1;
                    for (; index < runner.count && task->agents_count > sorted_tasks[index]->agents_count; ++index);
                    for (int i = 1; i < index; ++i)
                        sorted_tasks[i - 1] = sorted_tasks[i];
                    sorted_tasks[index - 1] = task;
                }

                cell = next_cell;
            }
        }

        /* set tasks */
        for (int i = 0; i < runner.count; ++i) {
            SeeTask *task = tasks + i;
            _init_tree_see_task(task, tree, pass, i);
            tay_thread_set_task(i, _see_func, task, pass->context);
        }
    }

    tay_runner_run();
}

typedef struct {
    CpuTree *tree;
    TayPass *pass;
    int counter; /* tree cell skipping counter so each thread processes different seer nodes */
} ActTask;

static void _init_tree_act_task(ActTask *task, CpuTree *tree, TayPass *pass, int thread_index) {
    task->tree = tree;
    task->pass = pass;
    task->counter = thread_index;
}

static void _thread_traverse_actors(ActTask *task, TreeCell *cell, TayThreadContext *thread_context) {
    if (task->counter % runner.count == 0) {
        if (cell->first) { /* if there are any "seeing" agents */
            for (TayAgentTag *tag = cell->first; tag; tag = tag->next)
                task->pass->act(tag, thread_context->context);
        }
    }
    ++task->counter;
    if (cell->lo)
        _thread_traverse_actors(task, cell->lo, thread_context);
    if (cell->hi)
        _thread_traverse_actors(task, cell->hi, thread_context);
}

static void _thread_act_traverse(ActTask *task, TayThreadContext *thread_context) {
    _thread_traverse_actors(task, task->tree->cells, thread_context);
}

void cpu_tree_act(TayPass *pass) {
    static ActTask tasks[TAY_MAX_THREADS];

    CpuTree *tree = &pass->act_space->cpu_tree;

    for (int i = 0; i < runner.count; ++i) {
        ActTask *task = tasks + i;
        _init_tree_act_task(task, tree, pass, i);
        tay_thread_set_task(i, _thread_act_traverse, task, pass->context);
    }

    tay_runner_run();
}

typedef struct {
    unsigned char arr[4];
} Depths;

void _tree_clear_cell(TreeCell *cell) {
    cell->lo = 0;
    cell->hi = 0;
    cell->first = 0;
    cell->count = 0;
}

static inline void _decide_cell_split(TreeCell *cell, int dims, int *max_depths, float *radii, Depths cell_depths) {
    cell->dim = TREE_CELL_LEAF_DIM;
    float max_r = 0.0f;
    for (int i = 0; i < dims; ++i) {
        if (cell_depths.arr[i] >= max_depths[i])
            continue;
        float r = (cell->box.max.arr[i] - cell->box.min.arr[i]) / radii[i];
        if (r > max_r) {
            max_r = r;
            cell->dim = i;
        }
    }
    if (cell->dim != TREE_CELL_LEAF_DIM)
        cell->mid = (cell->box.max.arr[cell->dim] + cell->box.min.arr[cell->dim]) * 0.5f;
}

static void _sort_point_agent(CpuTree *tree, TreeCell *cell, TayAgentTag *agent, Depths cell_depths, float *radii) {

    if (cell->dim == TREE_CELL_LEAF_DIM) { /* leaf cell, put the agent here */
        agent->next = cell->first;
        cell->first = agent;
        ++cell->count;
        return;
    }

    Depths sub_node_depths = cell_depths;
    ++sub_node_depths.arr[cell->dim];

    float pos = float4_agent_position(agent).arr[cell->dim];
    if (pos < cell->mid) {
        if (cell->lo == 0) { /* lo cell needed but doesn't exist yet */

            if (tree->cells_count == tree->max_cells) { /* no more space for new cells, leave the agent in current cell */
                agent->next = cell->first;
                cell->first = agent;
                ++cell->count;
                return;
            }

            cell->lo = tree->cells + tree->cells_count++;
            _tree_clear_cell(cell->lo);
            cell->lo->box = cell->box;
            cell->lo->box.max.arr[cell->dim] = cell->mid;
            _decide_cell_split(cell->lo, tree->dims, tree->max_depths.arr, radii, sub_node_depths);
        }
        _sort_point_agent(tree, cell->lo, agent, sub_node_depths, radii);
    }
    else {
        if (cell->hi == 0) { /* hi cell needed but doesn't exist yet */

            if (tree->cells_count == tree->max_cells) { /* no more space for new cells, leave the agent in current cell */
                agent->next = cell->first;
                cell->first = agent;
                ++cell->count;
                return;
            }

            cell->hi = tree->cells + tree->cells_count++;
            _tree_clear_cell(cell->hi);
            cell->hi->box = cell->box;
            cell->hi->box.min.arr[cell->dim] = cell->mid;
            _decide_cell_split(cell->hi, tree->dims, tree->max_depths.arr, radii, sub_node_depths);
        }
        _sort_point_agent(tree, cell->hi, agent, sub_node_depths, radii);
    }
}

static void _sort_non_point_agent(CpuTree *tree, TreeCell *cell, TayAgentTag *agent, Depths cell_depths, float *radii) {

    if (cell->dim == TREE_CELL_LEAF_DIM) { /* leaf cell, put the agent here */
        agent->next = cell->first;
        cell->first = agent;
        ++cell->count;
        return;
    }

    Depths sub_node_depths = cell_depths;
    ++sub_node_depths.arr[cell->dim];

    float min = float4_agent_min(agent).arr[cell->dim];
    float max = float4_agent_max(agent).arr[cell->dim];
    if (max <= cell->mid) {
        if (cell->lo == 0) { /* lo cell needed but doesn't exist yet */

            if (tree->cells_count == tree->max_cells) { /* no more space for new cells, leave the agent in current cell */
                agent->next = cell->first;
                cell->first = agent;
                ++cell->count;
                return;
            }

            cell->lo = tree->cells + tree->cells_count++;
            _tree_clear_cell(cell->lo);
            cell->lo->box = cell->box;
            cell->lo->box.max.arr[cell->dim] = cell->mid;
            _decide_cell_split(cell->lo, tree->dims, tree->max_depths.arr, radii, sub_node_depths);
        }
        _sort_non_point_agent(tree, cell->lo, agent, sub_node_depths, radii);
    }
    else if (min >= cell->mid) {
        if (cell->hi == 0) { /* hi cell needed but doesn't exist yet */

            if (tree->cells_count == tree->max_cells) { /* no more space for new cells, leave the agent in current cell */
                agent->next = cell->first;
                cell->first = agent;
                ++cell->count;
                return;
            }

            cell->hi = tree->cells + tree->cells_count++;
            _tree_clear_cell(cell->hi);
            cell->hi->box = cell->box;
            cell->hi->box.min.arr[cell->dim] = cell->mid;
            _decide_cell_split(cell->hi, tree->dims, tree->max_depths.arr, radii, sub_node_depths);
        }
        _sort_non_point_agent(tree, cell->hi, agent, sub_node_depths, radii);
    }
    else { /* agent extends to both sides of the mid plane, leave it in the current cell */
        agent->next = cell->first;
        cell->first = agent;
        ++cell->count;
#if TAY_TELEMETRY
        ++runner.telemetry.tree_branch_agents;
#endif
        return;
    }
}

static int _max_depth(float space_side, float cell_side) {
    int depth = 0;
    while ((space_side *= 0.5f) > cell_side)
        ++depth;
    return depth;
}

void cpu_tree_on_simulation_start(Space *space) {
    CpuTree *tree = &space->cpu_tree;
    tree->dims = space->dims;
    tree->cells = space->shared;
    tree->max_cells = space->shared_size / (int)sizeof(TreeCell);
    // ERROR: must be space for at least one cell
}

void cpu_tree_sort(TayGroup *group) {
    Space *space = &group->space;
    CpuTree *tree = &space->cpu_tree;

    /* calculate max partition depths for each dimension */
    Depths root_cell_depths;
    for (int i = 0; i < tree->dims; ++i) {
        tree->max_depths.arr[i] = _max_depth(space->box.max.arr[i] - space->box.min.arr[i], space->radii.arr[i] * 2.0f);
        root_cell_depths.arr[i] = 0;
    }

    /* set up root cell */
    {
        tree->cells_count = 1;
        _tree_clear_cell(tree->cells);
        tree->cells->box = space->box; /* root cell inherits tree's box */
        _decide_cell_split(tree->cells, tree->dims, tree->max_depths.arr, space->radii.arr, root_cell_depths);
    }

    TayAgentTag *next = space->first;

#if TAY_TELEMETRY
    runner.telemetry.tree_agents = 0;
    runner.telemetry.tree_branch_agents = 0;
#endif

    if (group->is_point) {
        while (next) {
            TayAgentTag *tag = next;
            next = next->next;
            _sort_point_agent(tree, tree->cells, tag, root_cell_depths, space->radii.arr);
#if TAY_TELEMETRY
            ++runner.telemetry.tree_agents;
#endif
        }
    }
    else {
        while (next) {
            TayAgentTag *tag = next;
            next = next->next;
            _sort_non_point_agent(tree, tree->cells, tag, root_cell_depths, space->radii.arr);
#if TAY_TELEMETRY
            ++runner.telemetry.tree_agents;
#endif
        }
    }

    space->first = 0;
    space->count = 0;
}

void cpu_tree_unsort(TayGroup *group) {
    Space *space = &group->space;
    CpuTree *tree = &space->cpu_tree;

    box_reset(&space->box, space->dims);

    for (int cell_i = 0; cell_i < tree->cells_count; ++cell_i) {
        TreeCell *cell = tree->cells + cell_i;
        space_return_agents(space, cell->first, group->is_point);
    }
}
