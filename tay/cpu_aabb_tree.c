#include "space.h"
#include "thread.h"


typedef struct AABBTreeCell {
    struct AABBTreeCell *l, *r;
    TayAgentTag *first[TAY_MAX_GROUPS];
    Box box;
} AABBTreeCell;

static void _clear_cell(AABBTreeCell *cell) {
    cell->l = 0;
    cell->r = 0;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        cell->first[i] = 0;
}

static float _evaluate_overlap(AABBTreeCell *a, AABBTreeCell *b)

static void _sort_agent(TayAgentTag *agent) {

}

void cpu_aabb_tree_sort(Space *space, TayGroup *groups) {
    CpuAABBTree *tree = &space->cpu_aabb_tree;

    tree->cells = space->shared;
    tree->max_cells = space->shared_size / (int)sizeof(AABBTreeCell);

    /* set up the root cell that encompasses the entire space */
    {
        tree->cells_count = 1;
        tree->cells->box = space->box;
        _clear_cell(tree->cells);
    }

    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = groups + group_i;

        if (group->space != space) /* sort only agents belonging to this space */
            continue;

        TayAgentTag *next = space->first[group_i];
        while (next) {
            TayAgentTag *agent = next;
            next = next->next;
            _sort_agent(agent);
        }

        space->first[group_i] = 0;
        space->counts[group_i] = 0;
    }
}
