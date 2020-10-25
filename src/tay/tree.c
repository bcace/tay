#include "tree.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <float.h>
#include <assert.h>

#define TREE_SPACE_CELL_LEAF_DIM       101


void tree_reset_box(Box *box, int dims) {
    for (int i = 0; i < dims; ++i) {
        box->min[i] = FLT_MAX;
        box->max[i] = -FLT_MAX;
    }
}

void tree_update_box(Box *box, float *p, int dims) {
    for (int i = 0; i < dims; ++i) {
        if (p[i] < box->min[i])
            box->min[i] = p[i];
        if (p[i] > box->max[i])
            box->max[i] = p[i];
    }
}

void tree_clear_cell(Cell *cell) {
    cell->lo = 0;
    cell->hi = 0;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        cell->first[i] = 0;
}

Space *tree_init(int dims, float *radii, int max_depth_correction) {
    Space *s = calloc(1, sizeof(Space));
    s->dims = dims;
    s->max_depth_correction = max_depth_correction;
    s->max_cells = 100000;
    s->cells = malloc(s->max_cells * sizeof(Cell));
    s->available_cell = 1; /* root cell is always allocated */
    tree_clear_cell(s->cells);
    tree_reset_box(&s->box, s->dims);
    for (int i = 0; i < dims; ++i)
        s->radii[i] = radii[i];
    return s;
}

void tree_destroy(Space *space) {
    free(space->cells);
    free(space);
}

static void _add_to_non_sorted(Space *space, TayAgentTag *agent, int group) {
    agent->next = space->first[group];
    space->first[group] = agent;
    tree_update_box(&space->box, TAY_AGENT_POSITION(agent), space->dims);
}

void tree_add(Space *space, TayAgentTag *agent, int group) {
    _add_to_non_sorted(space, agent, group);
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
            tree_update_box(&space->box, TAY_AGENT_POSITION(last), space->dims);
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
    cell->dim = TREE_SPACE_CELL_LEAF_DIM;
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
    if (cell->dim == TREE_SPACE_CELL_LEAF_DIM) {
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
                tree_clear_cell(cell->lo);
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
                tree_clear_cell(cell->hi);
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

void tree_update(Space *s) {
    _move_agents_from_cell_to_space(s, s->cells);

    /* calculate max partition depths for each dimension */
    Depths root_cell_depths;
    for (int i = 0; i < s->dims; ++i) {
        s->max_depths[i] = _max_depth(s->box.max[i] - s->box.min[i], s->radii[i] * 2.0f, s->max_depth_correction);
        root_cell_depths.dims[i] = 0;
    }

    /* set up root cell */
    s->available_cell = 1;
    tree_clear_cell(s->cells);
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

    tree_reset_box(&s->box, s->dims);
}
