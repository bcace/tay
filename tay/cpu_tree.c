#include "space.h"
#include "thread.h"
#include <string.h>

#define TREE_CELL_LEAF_DIM 100


typedef struct TreeCell {
    struct TreeCell *lo, *hi;       /* child partitions */
    unsigned first_agent_i;
    unsigned count;                 /* agent counts for this cell */
    int dim;                        /* dimension along which the cell's partition is divided into child partitions */
    Box box;
    float mid;
} TreeCell;

static inline void _init_cell(TreeCell *cell) {
    cell->lo = 0;
    cell->hi = 0;
    cell->count = 0;
}

static inline unsigned _cell_index(TreeCell *cells, TreeCell *cell) {
    return (unsigned)(cell - cells);
}

typedef struct {
    unsigned char arr[4];
} Depths;

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

static void _sort_point_agent(CpuKdTree *tree, TreeCell *cell, TayAgentTag *agent, Depths cell_depths, float *radii) {

    if (cell->dim == TREE_CELL_LEAF_DIM) { /* leaf cell, put the agent here */
        agent->part_i = _cell_index(tree->cells, cell);
        agent->cell_agent_i = cell->count;
        ++cell->count;
        return;
    }

    Depths sub_node_depths = cell_depths;
    ++sub_node_depths.arr[cell->dim];

    float pos = float4_agent_position(agent).arr[cell->dim];
    if (pos < cell->mid) {
        if (cell->lo == 0) { /* lo cell needed but doesn't exist yet */

            if (tree->cells_count == tree->max_cells) { /* no more space for new cells, leave the agent in current cell */
                agent->part_i = _cell_index(tree->cells, cell);
                agent->cell_agent_i = cell->count;
                ++cell->count;
                return;
            }

            cell->lo = tree->cells + tree->cells_count++;
            _init_cell(cell->lo);
            cell->lo->box = cell->box;
            cell->lo->box.max.arr[cell->dim] = cell->mid;
            _decide_cell_split(cell->lo, tree->dims, tree->max_depths.arr, radii, sub_node_depths);
        }
        _sort_point_agent(tree, cell->lo, agent, sub_node_depths, radii);
    }
    else {
        if (cell->hi == 0) { /* hi cell needed but doesn't exist yet */

            if (tree->cells_count == tree->max_cells) { /* no more space for new cells, leave the agent in current cell */
                agent->part_i = _cell_index(tree->cells, cell);
                agent->cell_agent_i = cell->count;
                ++cell->count;
                return;
            }

            cell->hi = tree->cells + tree->cells_count++;
            _init_cell(cell->hi);
            cell->hi->box = cell->box;
            cell->hi->box.min.arr[cell->dim] = cell->mid;
            _decide_cell_split(cell->hi, tree->dims, tree->max_depths.arr, radii, sub_node_depths);
        }
        _sort_point_agent(tree, cell->hi, agent, sub_node_depths, radii);
    }
}

static void _sort_non_point_agent(CpuKdTree *tree, TreeCell *cell, TayAgentTag *agent, Depths cell_depths, float *radii) {

    if (cell->dim == TREE_CELL_LEAF_DIM) { /* leaf cell, put the agent here */
        agent->part_i = _cell_index(tree->cells, cell);
        agent->cell_agent_i = cell->count;
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
                agent->part_i = _cell_index(tree->cells, cell);
                agent->cell_agent_i = cell->count;
                ++cell->count;
                return;
            }

            cell->lo = tree->cells + tree->cells_count++;
            _init_cell(cell->lo);
            cell->lo->box = cell->box;
            cell->lo->box.max.arr[cell->dim] = cell->mid;
            _decide_cell_split(cell->lo, tree->dims, tree->max_depths.arr, radii, sub_node_depths);
        }
        _sort_non_point_agent(tree, cell->lo, agent, sub_node_depths, radii);
    }
    else if (min >= cell->mid) {
        if (cell->hi == 0) { /* hi cell needed but doesn't exist yet */

            if (tree->cells_count == tree->max_cells) { /* no more space for new cells, leave the agent in current cell */
                agent->part_i = _cell_index(tree->cells, cell);
                agent->cell_agent_i = cell->count;
                ++cell->count;
                return;
            }

            cell->hi = tree->cells + tree->cells_count++;
            _init_cell(cell->hi);
            cell->hi->box = cell->box;
            cell->hi->box.min.arr[cell->dim] = cell->mid;
            _decide_cell_split(cell->hi, tree->dims, tree->max_depths.arr, radii, sub_node_depths);
        }
        _sort_non_point_agent(tree, cell->hi, agent, sub_node_depths, radii);
    }
    else { /* agent extends to both sides of the mid plane, leave it in the current cell */
        agent->part_i = _cell_index(tree->cells, cell);
        agent->cell_agent_i = cell->count;
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

// TODO: move this down to the sort function
void cpu_tree_on_simulation_start(Space *space) {
    CpuKdTree *tree = &space->cpu_tree;
    tree->dims = space->dims;
    tree->cells = space->shared;
    tree->max_cells = space->shared_size / (int)sizeof(TreeCell);
    /* ERROR: must be space for at least one cell */
}

static void _order_nodes(TreeCell *cell, unsigned *first_agent_i) {
    if (cell->count) {
        cell->first_agent_i = *first_agent_i;
        *first_agent_i += cell->count;
    }
    if (cell->lo)
        _order_nodes(cell->lo, first_agent_i);
    if (cell->hi)
        _order_nodes(cell->hi, first_agent_i);
}

void cpu_tree_sort(TayGroup *group) {
    Space *space = &group->space;
    CpuKdTree *tree = &space->cpu_tree;

    space_update_box(group);

    /* calculate max partition depths for each dimension */
    Depths root_cell_depths;
    for (int i = 0; i < tree->dims; ++i) {
        tree->max_depths.arr[i] = _max_depth(space->box.max.arr[i] - space->box.min.arr[i], space->radii.arr[i] * 2.0f);
        root_cell_depths.arr[i] = 0;
    }

    /* set up root cell */
    {
        tree->cells_count = 1;
        _init_cell(tree->cells);
        tree->cells->box = space->box; /* root cell inherits tree's box */
        _decide_cell_split(tree->cells, tree->dims, tree->max_depths.arr, space->radii.arr, root_cell_depths);
    }

#if TAY_TELEMETRY
    runner.telemetry.tree_agents = 0;
    runner.telemetry.tree_branch_agents = 0;
#endif

    if (group->is_point) {
        for (unsigned i = 0; i < space->count; ++i) {
            TayAgentTag *agent = (TayAgentTag *)(group->storage + group->agent_size * i);
            _sort_point_agent(tree, tree->cells, agent, root_cell_depths, space->radii.arr);
#if TAY_TELEMETRY
            ++runner.telemetry.tree_agents;
#endif
        }
    }
    else {
        for (unsigned i = 0; i < space->count; ++i) {
            TayAgentTag *agent = (TayAgentTag *)(group->storage + group->agent_size * i);
            _sort_non_point_agent(tree, tree->cells, agent, root_cell_depths, space->radii.arr);
#if TAY_TELEMETRY
            ++runner.telemetry.tree_agents;
#endif
        }
    }

    unsigned first_agent_i = 0;
    _order_nodes(tree->cells, &first_agent_i);

    for (unsigned i = 0; i < space->count; ++i) {
        TayAgentTag *src = (TayAgentTag *)(group->storage + group->agent_size * i);
        unsigned sorted_agent_i = tree->cells[src->part_i].first_agent_i + src->cell_agent_i;
        TayAgentTag *dst = (TayAgentTag *)(group->sort_storage + group->agent_size * sorted_agent_i);
        memcpy(dst, src, group->agent_size);
    }

    void *storage = group->storage;
    group->storage = group->sort_storage;
    group->sort_storage = storage;
}

void cpu_tree_unsort(TayGroup *group) {
}

typedef struct {
    TayPass *pass;
    int thread_i;
} ActTask;

static void _init_act_task(ActTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->thread_i = thread_i;
}

static void _act_func(ActTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;

    int agents_per_thread = pass->act_group->space.count / runner.count;
    unsigned beg_agent_i = agents_per_thread * task->thread_i;
    unsigned end_agent_i = (task->thread_i < runner.count - 1) ?
                           beg_agent_i + agents_per_thread :
                           pass->act_group->space.count;

    for (unsigned agent_i = beg_agent_i; agent_i < end_agent_i; ++agent_i) {
        void *agent = pass->act_group->storage + pass->act_group->agent_size * agent_i;
        pass->act(agent, thread_context->context);
    }
}

void cpu_tree_act(TayPass *pass) {
    static ActTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        ActTask *task = tasks + i;
        _init_act_task(task, pass, i);
        tay_thread_set_task(i, _act_func, task, pass->context);
    }

    tay_runner_run();
}

typedef struct {
    TayPass *pass;
    int thread_i;
} SeeTask;

static void _init_see_task(SeeTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->thread_i = thread_i;
}

static void _thread_traverse_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, TreeCell *seen_cell, int dims, TayThreadContext *thread_context) {
    for (int i = 0; i < dims; ++i)
        if (seer_box.min.arr[i] > seen_cell->box.max.arr[i] || seer_box.max.arr[i] < seen_cell->box.min.arr[i])
            return;

    if (seen_cell->count) {
        AgentsSlice seen_slice = {
            pass->seen_group->storage,
            pass->seen_group->agent_size,
            seen_cell->first_agent_i,
            seen_cell->first_agent_i + seen_cell->count
        };
        pass->pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
    }
    if (seen_cell->lo)
        _thread_traverse_seen(pass, seer_slice, seer_box, seen_cell->lo, dims, thread_context);
    if (seen_cell->hi)
        _thread_traverse_seen(pass, seer_slice, seer_box, seen_cell->hi, dims, thread_context);
}

void cpu_kd_tree_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, TayThreadContext *thread_context) {
    _thread_traverse_seen(pass, seer_slice, seer_box, pass->seen_group->space.cpu_tree.cells, dims, thread_context);
}

static inline unsigned _min(unsigned a, unsigned b) {
    return (a < b) ? a : b;
}

static void _see_func(SeeTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    TayGroup *seer_group = pass->seer_group;
    TayGroup *seen_group = pass->seen_group;
    CpuKdTree *seer_tree = &seer_group->space.cpu_tree;

    int min_dims = (seer_group->space.dims < seen_group->space.dims) ?
                   seer_group->space.dims :
                   seen_group->space.dims;

    int seers_per_thread = pass->seer_group->space.count / runner.count;
    unsigned beg_seer_i = seers_per_thread * task->thread_i;
    unsigned end_seer_i = (task->thread_i < runner.count - 1) ?
                          beg_seer_i + seers_per_thread :
                          pass->seer_group->space.count;

    unsigned seer_i = beg_seer_i;

    while (seer_i < end_seer_i) {
        TayAgentTag *seer = (TayAgentTag *)(seer_group->storage + seer_group->agent_size * seer_i);
        TreeCell *seer_cell = seer_tree->cells + seer->part_i;

        Box seer_box = seer_cell->box;
        for (int i = 0; i < min_dims; ++i) {
            seer_box.min.arr[i] -= pass->radii.arr[i];
            seer_box.max.arr[i] += pass->radii.arr[i];
        }

        unsigned cell_end_seer_i = _min(seer_cell->first_agent_i + seer_cell->count, end_seer_i);
        
        AgentsSlice seer_slice = {
            seer_group->storage,
            seer_group->agent_size,
            seer_i,
            cell_end_seer_i,
        };
        
        pass->seen_func(pass, seer_slice, seer_box, min_dims, thread_context);

        seer_i = cell_end_seer_i;
    }
}

void cpu_tree_see(TayPass *pass) {
    static SeeTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        SeeTask *task = tasks + i;
        _init_see_task(task, pass, i);
        tay_thread_set_task(i, _see_func, task, pass->context);
    }

    tay_runner_run();
}
