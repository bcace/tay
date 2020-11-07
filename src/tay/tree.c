#include "tree.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <float.h>
#include <assert.h>

// TODO: could turn to bool so it's a bit clearer
#define TREE_SPACE_CELL_LEAF_DIM 100


// TODO: this doesn't have to be a special struct then?
typedef struct {
    uchar4 _dims;
} Depths;

void tree_reset_box(Box *box, int dims) {
    for (int i = 0; i < dims; ++i) {
        box->min.arr[i] = FLT_MAX;
        box->max.arr[i] = -FLT_MAX;
    }
}

void tree_update_box(Box *box, float4 p, int dims) {
    for (int i = 0; i < dims; ++i) {
        if (p.arr[i] < box->min.arr[i])
            box->min.arr[i] = p.arr[i];
        if (p.arr[i] > box->max.arr[i])
            box->max.arr[i] = p.arr[i];
    }
}

void tree_clear_cell(Cell *cell) {
    cell->lo = 0;
    cell->hi = 0;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        cell->first[i] = 0;
}

void tree_init(TreeBase *tree, int dims, float4 radii, int max_depth_correction) {
    tree->dims = dims;
    tree->max_depth_correction = max_depth_correction;
    tree->max_cells = 100000;
    tree->cells = malloc(tree->max_cells * sizeof(Cell));
    tree->cells_count = 1; /* root cell is always allocated */
    tree_clear_cell(tree->cells);
    tree_reset_box(&tree->box, tree->dims);
    tree->radii = radii;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        tree->first[i] = 0;
}

void tree_destroy(TreeBase *tree) {
    free(tree->cells);
    free(tree);
}

static void _add_to_non_sorted(TreeBase *tree, TayAgentTag *agent, int group) {
    agent->next = tree->first[group];
    tree->first[group] = agent;
    tree_update_box(&tree->box, *TAY_AGENT_POSITION(agent), tree->dims);
}

void tree_add(TreeBase *tree, TayAgentTag *agent, int group) {
    _add_to_non_sorted(tree, agent, group);
}

static void _move_agents_from_cell_to_tree(TreeBase *tree, Cell *cell) {
    if (cell == 0)
        return;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        if (cell->first[i] == 0)
            continue;
        TayAgentTag *last = cell->first[i];
        /* update global extents and find the last agent */
        while (1) {
            tree_update_box(&tree->box, *TAY_AGENT_POSITION(last), tree->dims);
            if (last->next)
                last = last->next;
            else
                break;
        }
        /* add all agents to global list */
        last->next = tree->first[i];
        tree->first[i] = cell->first[i];
    }
    _move_agents_from_cell_to_tree(tree, cell->lo);
    _move_agents_from_cell_to_tree(tree, cell->hi);
}

static inline void _decide_cell_split(Cell *cell, int dims, int *max_depths, float *radii, Depths cell_depths) {
    cell->dim = TREE_SPACE_CELL_LEAF_DIM;
    float max_r = 0.0f;
    for (int i = 0; i < dims; ++i) {
        if (cell_depths._dims.arr[i] >= max_depths[i])
            continue;
        float r = (cell->box.max.arr[i] - cell->box.min.arr[i]) / radii[i];
        if (r > max_r) {
            max_r = r;
            cell->dim = i;
        }
    }
    if (cell->dim != TREE_SPACE_CELL_LEAF_DIM)
        cell->mid = (cell->box.max.arr[cell->dim] + cell->box.min.arr[cell->dim]) * 0.5f;
}

static void _sort_agent(TreeBase *tree, Cell *cell, TayAgentTag *agent, int group, Depths cell_depths) {
    if (cell->dim == TREE_SPACE_CELL_LEAF_DIM) {
        agent->next = cell->first[group];
        cell->first[group] = agent;
    }
    else {
        Depths sub_node_depths = cell_depths;
        ++sub_node_depths._dims.arr[cell->dim];

        float pos = TAY_AGENT_POSITION(agent)->arr[cell->dim];
        if (pos < cell->mid) {
            if (cell->lo == 0) {
                assert(tree->cells_count < tree->max_cells);
                cell->lo = tree->cells + tree->cells_count++;
                tree_clear_cell(cell->lo);
                cell->lo->box = cell->box;
                cell->lo->box.max.arr[cell->dim] = cell->mid;
                _decide_cell_split(cell->lo, tree->dims, tree->max_depths.arr, tree->radii.arr, sub_node_depths);
            }
            _sort_agent(tree, cell->lo, agent, group, sub_node_depths);
        }
        else {
            if (cell->hi == 0) {
                assert(tree->cells_count < tree->max_cells);
                cell->hi = tree->cells + tree->cells_count++;
                tree_clear_cell(cell->hi);
                cell->hi->box = cell->box;
                cell->hi->box.min.arr[cell->dim] = cell->mid;
                _decide_cell_split(cell->hi, tree->dims, tree->max_depths.arr, tree->radii.arr, sub_node_depths);
            }
            _sort_agent(tree, cell->hi, agent, group, sub_node_depths);
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

void tree_update(TreeBase *s) {
    _move_agents_from_cell_to_tree(s, s->cells);

    /* calculate max partition depths for each dimension */
    Depths root_cell_depths;
    for (int i = 0; i < s->dims; ++i) {
        s->max_depths.arr[i] = _max_depth(s->box.max.arr[i] - s->box.min.arr[i], s->radii.arr[i] * 2.0f, s->max_depth_correction);
        root_cell_depths._dims.arr[i] = 0;
    }

    /* set up root cell */
    s->cells_count = 1;
    tree_clear_cell(s->cells);
    s->cells->box = s->box; /* root cell inherits tree's box */
    _decide_cell_split(s->cells, s->dims, s->max_depths.arr, s->radii.arr, root_cell_depths);

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
