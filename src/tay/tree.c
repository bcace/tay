#include "space.h"
#include <assert.h>

#define TREE_CELL_LEAF_DIM 100


typedef struct {
    unsigned char arr[4];
} Depths;

void tree_clear_cell(TreeCell *cell) {
    cell->lo = 0;
    cell->hi = 0;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        cell->first[i] = 0;
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

static void _sort_agent(Tree *tree, TreeCell *cell, TayAgentTag *agent, int group, Depths cell_depths) {
    if (cell->dim == TREE_CELL_LEAF_DIM) {
        agent->next = cell->first[group];
        cell->first[group] = agent;
    }
    else {
        Depths sub_node_depths = cell_depths;
        ++sub_node_depths.arr[cell->dim];

        float pos = TAY_AGENT_POSITION(agent).arr[cell->dim];
        if (pos < cell->mid) {
            if (cell->lo == 0) {
                assert(tree->cells_count < TAY_MAX_TREE_CELLS);
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
                assert(tree->cells_count < TAY_MAX_TREE_CELLS);
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

static int _max_depth(float space_side, float cell_side, int depth_correction) {
    int depth = 0;
    while ((space_side *= 0.5f) > cell_side)
        ++depth;
    if ((depth += depth_correction) < 0)
        depth = 0;
    return depth;
}

void tree_update(Space *space) {
    Tree *tree = &space->cpu_tree.base;
    tree->dims = space->dims;
    tree->radii = space->radii;
    tree->cells = (TreeCell *)space->shared;

    assert(TAY_MAX_TREE_CELLS * sizeof(TreeCell) < TAY_GPU_ARENA_SIZE);

    /* calculate max partition depths for each dimension */
    Depths root_cell_depths;
    for (int i = 0; i < tree->dims; ++i) {
        tree->max_depths.arr[i] = _max_depth(space->box.max.arr[i] - space->box.min.arr[i], tree->radii.arr[i] * 2.0f, space->depth_correction);
        root_cell_depths.arr[i] = 0;
    }

    /* set up root cell */
    tree->cells_count = 1;
    tree_clear_cell(tree->cells);
    tree->cells->box = space->box; /* root cell inherits tree's box */
    _decide_cell_split(tree->cells, tree->dims, tree->max_depths.arr, tree->radii.arr, root_cell_depths);

    /* sort all agents from tree into nodes */
    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayAgentTag *next = space->first[group_i];
        while (next) {
            TayAgentTag *tag = next;
            next = next->next;
            _sort_agent(tree, tree->cells, tag, group_i, root_cell_depths);
        }
        space->first[group_i] = 0;
        space->counts[group_i] = 0;
    }

    box_reset(&space->box, tree->dims);
}

static void _return_agents(Space *space, TreeCell *cell) {
    if (cell == 0)
        return;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        space_return_agents(space, i, cell->first[i]);
    _return_agents(space, cell->lo);
    _return_agents(space, cell->hi);
}

void tree_return_agents(Space *space) {
    _return_agents(space, space->cpu_tree.base.cells);
}
