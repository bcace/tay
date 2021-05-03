#include "space.h"
#include "thread.h"
#include <string.h>
#include <math.h>


typedef struct ZGridCell {
    unsigned count;
    unsigned first_agent_i;
} ZGridCell;

static inline uint4 _agent_position_to_cell_indices(float4 pos, float4 orig, float4 sizes, int dims) {
    uint4 indices;
    switch (dims) {
        case 1: {
            indices.x = (unsigned)floorf((pos.x - orig.x) / sizes.x);
        } break;
        case 2: {
            indices.x = (unsigned)floorf((pos.x - orig.x) / sizes.x);
            indices.y = (unsigned)floorf((pos.y - orig.y) / sizes.y);
        } break;
        case 3: {
            indices.x = (unsigned)floorf((pos.x - orig.x) / sizes.x);
            indices.y = (unsigned)floorf((pos.y - orig.y) / sizes.y);
            indices.z = (unsigned)floorf((pos.z - orig.z) / sizes.z);
        } break;
        default: {
            indices.x = (unsigned)floorf((pos.x - orig.x) / sizes.x);
            indices.y = (unsigned)floorf((pos.y - orig.y) / sizes.y);
            indices.z = (unsigned)floorf((pos.z - orig.z) / sizes.z);
            indices.w = (unsigned)floorf((pos.w - orig.w) / sizes.w);
        };
    }
    return indices;
}

static inline unsigned _cell_indices_to_cell_index(uint4 idx, int dims) {
    if (dims == 1) {
        return idx.x;
    }
    else if (dims ==  2) { /* 16 bits */
        idx.x = (idx.x | (idx.x << 8)) & 0x00FF00FF;
        idx.x = (idx.x | (idx.x << 4)) & 0x0F0F0F0F;
        idx.x = (idx.x | (idx.x << 2)) & 0x33333333;
        idx.x = (idx.x | (idx.x << 1)) & 0x55555555;

        idx.y = (idx.y | (idx.y << 8)) & 0x00FF00FF;
        idx.y = (idx.y | (idx.y << 4)) & 0x0F0F0F0F;
        idx.y = (idx.y | (idx.y << 2)) & 0x33333333;
        idx.y = (idx.y | (idx.y << 1)) & 0x55555555;

        return idx.x | (idx.y << 1);
    }
    else if (dims == 3) { /* 10 bits */
        idx.x = (idx.x | (idx.x << 16)) & 0x030000FF;
        idx.x = (idx.x | (idx.x <<  8)) & 0x0300F00F;
        idx.x = (idx.x | (idx.x <<  4)) & 0x030C30C3;
        idx.x = (idx.x | (idx.x <<  2)) & 0x09249249;

        idx.y = (idx.y | (idx.y << 16)) & 0x030000FF;
        idx.y = (idx.y | (idx.y <<  8)) & 0x0300F00F;
        idx.y = (idx.y | (idx.y <<  4)) & 0x030C30C3;
        idx.y = (idx.y | (idx.y <<  2)) & 0x09249249;

        idx.z = (idx.z | (idx.z << 16)) & 0x030000FF;
        idx.z = (idx.z | (idx.z <<  8)) & 0x0300F00F;
        idx.z = (idx.z | (idx.z <<  4)) & 0x030C30C3;
        idx.z = (idx.z | (idx.z <<  2)) & 0x09249249;

        return idx.x | (idx.y << 1) | (idx.z << 2);
    }
    else {
        /* not implemented */
        return 0;
    }
}

static inline unsigned _round_down_to_power_of_2(unsigned v) {
    unsigned n = 0u;
    while ((1u << (n + 1u)) < v)
        ++n;
    return n;
}

static inline unsigned _powi(unsigned v, int dims) {
    unsigned p = 1u;
    for (int i = 0; i < dims; ++i)
        p *= v;
    return p;
}

void cpu_z_grid_on_simulation_start(Space *space) {
    CpuZGrid *grid = &space->cpu_z_grid;
    grid->cells = space->shared;
    grid->max_cells = space->shared_size / (unsigned)sizeof(ZGridCell);
    memset(space->cpu_z_grid.cells, 0, space->cpu_z_grid.max_cells * sizeof(ZGridCell));
}

void cpu_z_grid_sort(TayGroup *group) {
    Space *space = &group->space;
    CpuZGrid *grid = &space->cpu_z_grid;

    space_update_box(group);

    unsigned n = 1 << 31;

    for (int i = 0; i < space->dims; ++i) {
        float cell_size = space->radii.arr[i] * 2.0f;
        float space_size = space->box.max.arr[i] - space->box.min.arr[i] + cell_size * 0.001f;

        unsigned cells_count = (unsigned)floorf(space_size / cell_size);
        unsigned side_n = _round_down_to_power_of_2(cells_count);

        if (side_n < n)
            n = side_n;
    }

    while (_powi(1u << n, space->dims) > grid->max_cells)
        --n;

    grid->origin = space->box.min;

    for (int i = 0; i < space->dims; ++i) {
        float cell_size = space->radii.arr[i] * 2.0f;
        float space_size = space->box.max.arr[i] - space->box.min.arr[i] + cell_size * 0.001f;

        grid->cell_counts.arr[i] = 1u << n;
        grid->cell_sizes.arr[i] = space_size / (float)grid->cell_counts.arr[i];
    }

    grid->cells_count = _powi(1u << n, space->dims);

    /* ............................ */

    for (unsigned i = 0; i < space->count; ++i) {
        TayAgentTag *agent = (TayAgentTag *)(group->storage + group->agent_size * i);
        float4 p = float4_agent_position(agent);

        uint4 indices = _agent_position_to_cell_indices(p, grid->origin, grid->cell_sizes, space->dims);
        unsigned cell_i = _cell_indices_to_cell_index(indices, space->dims);

        ZGridCell *cell = grid->cells + cell_i;
        agent->part_i = cell_i;
        agent->cell_agent_i = cell->count++;
    }

    unsigned first_agent_i = 0;
    for (unsigned cell_i = 0; cell_i < grid->cells_count; ++cell_i) {
        ZGridCell *cell = grid->cells + cell_i;
        cell->first_agent_i = first_agent_i;
        first_agent_i += cell->count;
    }

    for (unsigned i = 0; i < space->count; ++i) {
        TayAgentTag *src = (TayAgentTag *)(group->storage + group->agent_size * i);
        unsigned sorted_agent_i = grid->cells[src->part_i].first_agent_i + src->cell_agent_i;
        TayAgentTag *dst = (TayAgentTag *)(group->sort_storage + group->agent_size * sorted_agent_i);
        memcpy(dst, src, group->agent_size);
    }

    void *storage = group->storage;
    group->storage = group->sort_storage;
    group->sort_storage = storage;
}

void cpu_z_grid_unsort(TayGroup *group) {
    memset(group->space.cpu_z_grid.cells, 0, group->space.cpu_z_grid.cells_count * sizeof(ZGridCell));
}
