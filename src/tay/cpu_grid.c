#include "space.h"
#include <math.h>


typedef struct GridCell {
    struct GridCell *next;
    TayAgentTag *first[TAY_MAX_GROUPS];
    int used;
} GridCell;

static inline int _agent_position_to_cell_index(float p, float o, float s) {
    return (int)floorf((p - o) / s);
}

static int _agent_position_to_hash(CpuGrid *grid, float4 p, int dims) {
    unsigned long long h = 0;
    static unsigned long long c[] = { 1640531513, 2654435789, 2147483647, 6692367337 };
    for (int i = 0; i < dims; ++i) {
        unsigned long long index = _agent_position_to_cell_index(p.arr[i], grid->grid_origin.arr[i], grid->cell_sizes.arr[i]);
        h ^= index * c[i];
    }
    return h % TAY_MAX_CELLS;
}

void cpu_grid_prepare(TayState *state) {
    Space *space = &state->space;
    CpuGrid *grid = &space->cpu_grid;
    grid->cells = space_get_cell_arena(space, TAY_MAX_CELLS * sizeof(GridCell), 1);
}

void cpu_grid_step(TayState *state) {
    Space *space = &state->space;
    CpuGrid *grid = &space->cpu_grid;

    /* calculate grid parameters */
    for (int i = 0; i < space->dims; ++i) {
        float cell_size = space->radii.arr[i] * 2.0f;
        float space_size = space->box.max.arr[i] - space->box.min.arr[i];
        int count = (int)ceilf(space_size / cell_size);
        float margin = (count * cell_size - space_size) * 0.5f;
        grid->cell_counts.arr[i] = count;
        grid->cell_sizes.arr[i] = cell_size;
        grid->grid_origin.arr[i] = space->box.min.arr[i] - margin;
    }

    /* sort agents into cells */
    {
        grid->first = 0;

        for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            TayAgentTag *next = space->first[group_i];
            while (next) {
                TayAgentTag *tag = next;
                next = next->next;

                /* sort agent */
                {
                    int hash = _agent_position_to_hash(grid, TAY_AGENT_POSITION(tag), space->dims);
                    GridCell *cell = grid->cells + hash;

                    tag->next = cell->first[group_i];
                    cell->first[group_i] = tag;

                    if (cell->used == 0) {
                        cell->next = grid->first;
                        grid->first = cell;
                        cell->used = 1;
                    }
                }
            }
            space->first[group_i] = 0;
            space->counts[group_i] = 0;
        }
    }
}
