#include "state.h"
#include "gpu.h"
#include <stdlib.h>
#include <assert.h>
#include <float.h>


typedef enum {
    CELL_DIM_LEAF = 101
} CellDim;

#pragma pack(push, 1)
typedef struct {
    float min[TAY_MAX_DIMENSIONS];
    float max[TAY_MAX_DIMENSIONS];
} Box;
#pragma pack(pop)

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

#pragma pack(push, 1)
typedef struct {
    int dims[TAY_MAX_DIMENSIONS];
} Depths;
#pragma pack(pop)

static void _reset_depths(Depths *depths, int dims) {
    for (int i = 0; i < dims; ++i)
        depths->dims[i] = 0;
}

#pragma pack(push, 1)
typedef struct Cell {
    struct Cell *lo, *hi;
    TayAgentTag *first[TAY_MAX_GROUPS];
    Box box;
    int dim;                            /* dimension along which the node's partition is divided into child partitions */
    Depths depths;
} Cell;
#pragma pack(pop)

static void _clear_cell(Cell *cell) {
    cell->lo = 0;
    cell->hi = 0;
    cell->dim = CELL_DIM_LEAF;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        cell->first[i] = 0;
}

typedef struct {
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents to be added to the space at next update */
    Box box;
    Cell *cells;
    int max_cells;
    int available_cell;
    int dims;
    int max_depth_correction;
    float radii[TAY_MAX_DIMENSIONS];
    int max_depths[TAY_MAX_DIMENSIONS];
} Space;

static Space *_init(int dims, float *radii, int max_depth_correction) {
    Space *s = malloc(sizeof(Space));
    s->dims = dims;
    s->max_depth_correction = max_depth_correction;
    s->max_cells = 100000;
    s->cells = malloc(sizeof(Cell) * s->max_cells);
    s->available_cell = 1; /* first cell is always the root cell */
    _clear_cell(s->cells);
    _reset_box(&s->box, dims);
    for (int i = 0; i < dims; ++i)
        s->radii[i] = radii[i];
    return s;
}

static void _destroy(TaySpaceContainer *container) {
    Space *s = container->storage;
    free(s->cells);
    free(s);
}

static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
    Space *space = container->storage;
    agent->next = space->first[group];
    space->first[group] = agent;
    _update_box(&space->box, TAY_AGENT_POSITION(agent), space->dims);
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

static inline void _decide_cell_split(Cell *cell, int dims, int *max_depths, float *radii) {
    cell->dim = CELL_DIM_LEAF;
    float max_r = 0.0f;
    for (int i = 0; i < dims; ++i) { /* do we subdivide it further? */
        if (cell->depths.dims[i] >= max_depths[i])
            continue;
        float r = (cell->box.max[i] - cell->box.min[i]) / radii[i];
        if (r > max_r) {
            max_r = r;
            cell->dim = i;
        }
    }
}

static void _sort_agent(Space *space, Cell *cell, TayAgentTag *agent, int group) {
    if (cell->dim == CELL_DIM_LEAF) { /* if leaf, add agent to it */
        agent->next = cell->first[group];
        cell->first[group] = agent;
    }
    else { /* if branch */
        float mid = (cell->box.max[cell->dim] + cell->box.min[cell->dim]) * 0.5f;
        float pos = TAY_AGENT_POSITION(agent)[cell->dim];
        if (pos < mid) {
            if (cell->lo == 0) {
                assert(space->available_cell < space->max_cells);
                cell->lo = space->cells + space->available_cell++;
                _clear_cell(cell->lo);
                cell->lo->box = cell->box;
                cell->lo->box.max[cell->dim] = mid;
                cell->lo->depths = cell->depths;
                cell->lo->depths.dims[cell->dim]++;
                _decide_cell_split(cell->lo, space->dims, space->max_depths, space->radii);
            }
            _sort_agent(space, cell->lo, agent, group);
        }
        else {
            if (cell->hi == 0) {
                assert(space->available_cell < space->max_cells);
                cell->hi = space->cells + space->available_cell++;
                _clear_cell(cell->hi);
                cell->hi->box = cell->box;
                cell->hi->box.min[cell->dim] = mid;
                cell->hi->depths = cell->depths;
                cell->hi->depths.dims[cell->dim]++;
                _decide_cell_split(cell->hi, space->dims, space->max_depths, space->radii);
            }
            _sort_agent(space, cell->hi, agent, group);
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
    _clear_cell(s->cells);
    s->cells->box = s->box;
    s->available_cell = 1;
    _reset_depths(&s->cells->depths, s->dims);

    for (int i = 0; i < s->dims; ++i)
        s->max_depths[i] = _max_depth(s->box.max[i] - s->box.min[i], s->radii[i] * 2.0f, s->max_depth_correction);

    /* sort all agents from tree into nodes */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayAgentTag *next = s->first[i];
        while (next) {
            TayAgentTag *a = next;
            next = next->next;
            _sort_agent(s, s->cells, a, i);
        }
        s->first[i] = 0;
    }

    _reset_box(&s->box, s->dims);
}

void space_gpu_tree_init(TaySpaceContainer *container, int dims, float *radii, int max_depth_correction) {
    space_container_init(container, _init(dims, radii, max_depth_correction), dims, _destroy);
    container->add = _add;
    container->on_step_start = _on_step_start;
}
