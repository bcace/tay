#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#define NODE_DIM_LEAF       101


typedef struct {
    float min[TAY_MAX_DIMENSIONS];
    float max[TAY_MAX_DIMENSIONS];
} Box;

static void _reset_box(Box *box, int dims) {
    for (int i = 0; i < dims; ++i) {
        box->min[i] = FLT_MAX;
        box->max[i] = -FLT_MAX;
    }
}

static void _update_box(Box *box, float *p, int dims) {
    for (int i = 0; i < dims; ++i) {
        if (p[i] < box->min[i])
            box->min[i] = p[i];
        if (p[i] > box->max[i])
            box->max[i] = p[i];
    }
}

typedef struct {
    unsigned char dims[TAY_MAX_DIMENSIONS];
} Depths;

typedef struct Cell {
    struct Cell *lo, *hi;               /* child partitions */
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents contained in this cell (fork or leaf) */
    int dim;                            /* dimension along which the cell's partition is divided into child partitions */
    Box box;
} Cell;

static void _clear_cell(Cell *cell) {
    cell->lo = 0;
    cell->hi = 0;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        cell->first[i] = 0;
}

typedef struct {
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents are kept here while not in cells, will be sorted into the tree at next step */
    Cell *cells;                        /* cells storage, first cell is always the root cell */
    int max_cells;
    int available_cell;
    int dims;
    int max_depth_correction;
    float radii[TAY_MAX_DIMENSIONS];
    int max_depths[TAY_MAX_DIMENSIONS];
    Box box;
} Space;

static Space *_init(int dims, float *radii, int max_depth_correction) {
    Space *s = calloc(1, sizeof(Space));
    s->dims = dims;
    s->max_depth_correction = max_depth_correction;
    s->max_cells = 100000;
    s->cells = malloc(s->max_cells * sizeof(Cell));
    s->available_cell = 1; /* root cell is always allocated */
    _clear_cell(s->cells);
    _reset_box(&s->box, s->dims);
    for (int i = 0; i < dims; ++i)
        s->radii[i] = radii[i];
    return s;
}

static void _destroy(TaySpaceContainer *container) {
    Space *s = container->storage;
    free(s->cells);
    free(s);
}

static void _add_to_non_sorted(Space *space, TayAgentTag *agent, int group) {
    agent->next = space->first[group];
    space->first[group] = agent;
    _update_box(&space->box, TAY_AGENT_POSITION(agent), space->dims);
}

static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
    Space *s = container->storage;
    _add_to_non_sorted(s, agent, group);
}

static void _move_agents_from_cell_to_space(Space *space, Cell *cell) {
    if (cell == 0)
        return;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        if (cell->first[i] == 0)
            continue;
        TayAgentTag *last = cell->first[i];
        /* update global extents and find the last agent */
        while (1) {
            _update_box(&space->box, TAY_AGENT_POSITION(last), space->dims);
            if (last->next)
                last = last->next;
            else
                break;
        }
        /* add all agents to global list */
        last->next = space->first[i];
        space->first[i] = cell->first[i];
    }
    _move_agents_from_cell_to_space(space, cell->lo);
    _move_agents_from_cell_to_space(space, cell->hi);
}

static inline void _decide_cell_split(Cell *cell, int dims, int *max_depths, float *radii, Depths cell_depths) {
    cell->dim = NODE_DIM_LEAF;
    float max_r = 0.0f;
    for (int i = 0; i < dims; ++i) {
        if (cell_depths.dims[i] >= max_depths[i])
            continue;
        float r = (cell->box.max[i] - cell->box.min[i]) / radii[i];
        if (r > max_r) {
            max_r = r;
            cell->dim = i;
        }
    }
}

static void _sort_agent(Space *space, Cell *cell, TayAgentTag *agent, int group, Depths cell_depths) {
    if (cell->dim == NODE_DIM_LEAF) {
        agent->next = cell->first[group];
        cell->first[group] = agent;
    }
    else {
        Depths sub_node_depths = cell_depths;
        ++sub_node_depths.dims[cell->dim];

        float mid = (cell->box.max[cell->dim] + cell->box.min[cell->dim]) * 0.5f;
        float pos = TAY_AGENT_POSITION(agent)[cell->dim];
        if (pos < mid) {
            if (cell->lo == 0) {
                assert(space->available_cell < space->max_cells);
                cell->lo = space->cells + space->available_cell++;
                _clear_cell(cell->lo);
                cell->lo->box = cell->box;
                cell->lo->box.max[cell->dim] = mid;
                _decide_cell_split(cell->lo, space->dims, space->max_depths, space->radii, sub_node_depths);
            }
            _sort_agent(space, cell->lo, agent, group, sub_node_depths);
        }
        else {
            if (cell->hi == 0) {
                assert(space->available_cell < space->max_cells);
                cell->hi = space->cells + space->available_cell++;
                _clear_cell(cell->hi);
                cell->hi->box = cell->box;
                cell->hi->box.min[cell->dim] = mid;
                _decide_cell_split(cell->hi, space->dims, space->max_depths, space->radii, sub_node_depths);
            }
            _sort_agent(space, cell->hi, agent, group, sub_node_depths);
        }
    }
}

static int _max_depth(float space_side, float see_side, int max_depth_correction) {
    int depth = 0;
    while ((space_side *= 0.5f) > see_side)
        ++depth;
    if ((depth += max_depth_correction) < 0)
        depth = 0;
    return depth;
}

static void _on_step_start(TayState *state) {
    Space *s = state->space.storage;

    _move_agents_from_cell_to_space(s, s->cells);

    /* calculate max partition depths for each dimension */
    Depths root_cell_depths;
    for (int i = 0; i < s->dims; ++i) {
        s->max_depths[i] = _max_depth(s->box.max[i] - s->box.min[i], s->radii[i] * 2.0f, s->max_depth_correction);
        root_cell_depths.dims[i] = 0;
    }

    /* set up root cell */
    s->available_cell = 1;
    _clear_cell(s->cells);
    s->cells->box = s->box; /* root cell inherits space's box */
    _decide_cell_split(s->cells, s->dims, s->max_depths, s->radii, root_cell_depths);

    /* sort all agents from tree into nodes */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayAgentTag *next = s->first[i];
        while (next) {
            TayAgentTag *a = next;
            next = next->next;
            _sort_agent(s, s->cells, a, i, root_cell_depths);
        }
        s->first[i] = 0;
    }

    _reset_box(&s->box, s->dims);
}

typedef struct {
    Space *space;
    TayPass *pass;
    int counter; /* tree cell skipping counter so each thread processes different seer nodes */
} SeeTask;

static void _init_tree_see_task(SeeTask *t, Space *space, TayPass *pass, int thread_index) {
    t->space = space;
    t->pass = pass;
    t->counter = thread_index;
}

static void _thread_traverse_seen(Space *space, Cell *seer_node, Cell *seen_node, TayPass *pass, Box seer_box, TayThreadContext *thread_context) {
    for (int i = 0; i < space->dims; ++i)
        if (seer_box.min[i] > seen_node->box.max[i] || seer_box.max[i] < seen_node->box.min[i])
            return;
    if (seen_node->first[pass->seen_group]) /* if there are any "seen" agents */
        tay_see(seer_node->first[pass->seer_group], seen_node->first[pass->seen_group], pass->see, pass->radii, space->dims, thread_context);
    if (seen_node->lo)
        _thread_traverse_seen(space, seer_node, seen_node->lo, pass, seer_box, thread_context);
    if (seen_node->hi)
        _thread_traverse_seen(space, seer_node, seen_node->hi, pass, seer_box, thread_context);
}

static void _thread_traverse_seers(SeeTask *task, Cell *cell, TayThreadContext *thread_context) {
    if (task->counter % runner.count == 0) {
        if (cell->first[task->pass->seer_group]) { /* if there are any "seeing" agents */
            Box seer_box = cell->box;
            for (int i = 0; i < task->space->dims; ++i) {
                seer_box.min[i] -= task->pass->radii[i];
                seer_box.max[i] += task->pass->radii[i];
            }
            _thread_traverse_seen(task->space, cell, task->space->cells, task->pass, seer_box, thread_context);
        }
    }
    ++task->counter;
    if (cell->lo)
        _thread_traverse_seers(task, cell->lo, thread_context);
    if (cell->hi)
        _thread_traverse_seers(task, cell->hi, thread_context);
}

static void _thread_see_traverse(SeeTask *task, TayThreadContext *thread_context) {
    _thread_traverse_seers(task, task->space->cells, thread_context);
}

static void _see(TayState *state, int pass_index) { // TaySpaceContainer *container, TayPass *pass) {
    Space *t = state->space.storage; // container->storage;
    TayPass *p = state->passes + pass_index;

    static SeeTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        SeeTask *task = tasks + i;
        _init_tree_see_task(task, t, p, i);
        tay_thread_set_task(i, _thread_see_traverse, task, p->context);
    }

    tay_runner_run();
}

typedef struct {
    TayPass *pass;
    TayAgentTag *agents;
} ActTask;

static void _init_tree_act_task(ActTask *t, TayPass *pass, TayAgentTag *agents) {
    t->pass = pass;
    t->agents = agents;
}

static void _act_func(ActTask *t, TayThreadContext *thread_context) {
    for (TayAgentTag *a = t->agents; a; a = a->next)
        t->pass->act(a, thread_context->context);
}

static void _dummy_act_func(ActTask *t, TayThreadContext *thread_context) {}

static void _traverse_actors(Cell *cell, TayPass *pass) {
    ActTask task;
    _init_tree_act_task(&task, pass, cell->first[pass->act_group]);
    tay_thread_set_task(0, _act_func, &task, pass->context);
    for (int i = 1; i < runner.count; ++i)
        tay_thread_set_task(i, _dummy_act_func, 0, 0); // TODO: this is horrible, act here is still not parallelized!!!
    tay_runner_run_no_threads();
    if (cell->lo)
        _traverse_actors(cell->lo, pass);
    if (cell->hi)
        _traverse_actors(cell->hi, pass);
}

static void _act(TayState *state, int pass_index) {
    Space *s = state->space.storage;
    TayPass *p = state->passes + pass_index;
    _traverse_actors(s->cells, p);
}

void space_tree_init(TaySpaceContainer *container, int dims, float *radii, int max_depth_correction) {
    space_container_init(container, _init(dims, radii, max_depth_correction), dims, _destroy);
    container->add = _add;
    container->see = _see,
    container->act = _act;
    container->on_step_start = _on_step_start;
}
