#include "space.h"
#include "thread.h"
#include <string.h>
#include <math.h>


typedef struct ZGridCell {
    unsigned count;
    unsigned first_agent_i;
} ZGridCell;

static inline int4 _position_to_cell_indices(float4 pos, float4 orig, float4 sizes, int dims) {
    int4 indices;
    switch (dims) {
        case 1: {
            indices.x = (int)floorf((pos.x - orig.x) / sizes.x);
        } break;
        case 2: {
            indices.x = (int)floorf((pos.x - orig.x) / sizes.x);
            indices.y = (int)floorf((pos.y - orig.y) / sizes.y);
        } break;
        case 3: {
            indices.x = (int)floorf((pos.x - orig.x) / sizes.x);
            indices.y = (int)floorf((pos.y - orig.y) / sizes.y);
            indices.z = (int)floorf((pos.z - orig.z) / sizes.z);
        } break;
        default: {
            indices.x = (int)floorf((pos.x - orig.x) / sizes.x);
            indices.y = (int)floorf((pos.y - orig.y) / sizes.y);
            indices.z = (int)floorf((pos.z - orig.z) / sizes.z);
            indices.w = (int)floorf((pos.w - orig.w) / sizes.w);
        };
    }
    return indices;
}

static inline unsigned _cell_indices_to_cell_index(int4 idx, int dims) {
    if (dims == 1) {
        return idx.x;
    }
    else if (dims ==  2) { /* 16 bits */
        unsigned x = idx.x;
        unsigned y = idx.y;

        x = (x | (x << 8)) & 0x00FF00FF;
        x = (x | (x << 4)) & 0x0F0F0F0F;
        x = (x | (x << 2)) & 0x33333333;
        x = (x | (x << 1)) & 0x55555555;

        y = (y | (y << 8)) & 0x00FF00FF;
        y = (y | (y << 4)) & 0x0F0F0F0F;
        y = (y | (y << 2)) & 0x33333333;
        y = (y | (y << 1)) & 0x55555555;

        return x | (y << 1);
    }
    else if (dims == 3) { /* 10 bits */
        unsigned x = idx.x;
        unsigned y = idx.y;
        unsigned z = idx.z;

        x = (x | (x << 16)) & 0x030000FF;
        x = (x | (x <<  8)) & 0x0300F00F;
        x = (x | (x <<  4)) & 0x030C30C3;
        x = (x | (x <<  2)) & 0x09249249;

        y = (y | (y << 16)) & 0x030000FF;
        y = (y | (y <<  8)) & 0x0300F00F;
        y = (y | (y <<  4)) & 0x030C30C3;
        y = (y | (y <<  2)) & 0x09249249;

        z = (z | (z << 16)) & 0x030000FF;
        z = (z | (z <<  8)) & 0x0300F00F;
        z = (z | (z <<  4)) & 0x030C30C3;
        z = (z | (z <<  2)) & 0x09249249;

        return x | (y << 1) | (z << 2);
    }
    else {
        /* not implemented */
        return 0;
    }
}

static inline int4 _cell_index_to_cell_indices(unsigned i, int dims) {
    unsigned x, y, z;
    if (dims == 1) {
        x = i;
    }
    else if (dims == 2) {
        x = i & 0x55555555;
        x = (x | (x >> 1)) & 0x33333333;
        x = (x | (x >> 2)) & 0x0f0f0f0f;
        x = (x | (x >> 4)) & 0x00ff00ff;
        x = (x | (x >> 8)) & 0x0000ffff;

        y = (i >> 1) & 0x55555555;
        y = (y | (y >> 1)) & 0x33333333;
        y = (y | (y >> 2)) & 0x0f0f0f0f;
        y = (y | (y >> 4)) & 0x00ff00ff;
        y = (y | (y >> 8)) & 0x0000ffff;
    }
    else if (dims == 3) {
        x = i & 0x09249249;
        x = (x | (x >>  2)) & 0x030c30c3;
        x = (x | (x >>  4)) & 0x0300f00f;
        x = (x | (x >>  8)) & 0xff0000ff;
        x = (x | (x >> 16)) & 0x000003ff;

        y = (i >> 1) & 0x09249249;
        y = (y | (y >>  2)) & 0x030c30c3;
        y = (y | (y >>  4)) & 0x0300f00f;
        y = (y | (y >>  8)) & 0xff0000ff;
        y = (y | (y >> 16)) & 0x000003ff;

        z = (i >> 2) & 0x09249249;
        z = (z | (z >>  2)) & 0x030c30c3;
        z = (z | (z >>  4)) & 0x0300f00f;
        z = (z | (z >>  8)) & 0xff0000ff;
        z = (z | (z >> 16)) & 0x000003ff;
    }
    else {
        /* not implemented */
    }
    return (int4){x, y, z};
}

static inline unsigned _round_down_to_power_of_2(unsigned v) {
    unsigned n = 0u;
    while ((1u << (n + 1u)) <= v)
        ++n;
    return n;
}

static inline unsigned _round_up_to_power_of_2(unsigned v) {
    unsigned n = 0u;
    while ((1u << n) <= v)
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

    unsigned n = 0;

    for (int i = 0; i < space->dims; ++i) {
        float cell_size = space->radii.arr[i] * 2.0f;
        float space_size = space->box.max.arr[i] - space->box.min.arr[i] + cell_size * 0.001f;

        unsigned cells_count = (unsigned)floorf(space_size / cell_size);
        unsigned side_n = _round_down_to_power_of_2(cells_count);

        if (side_n > n)
            n = side_n;
    }

    while (_powi(1u << (n + 1u), space->dims) > grid->max_cells)
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

        int4 indices = _position_to_cell_indices(p, grid->origin, grid->cell_sizes, space->dims);
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

typedef struct {
    unsigned beg;
    unsigned end;
} ZGridKernelCell;

void cpu_z_grid_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, struct TayThreadContext *thread_context) {
    CpuZGrid *seen_grid = &pass->seen_group->space.cpu_z_grid;

    int4 min_indices = _position_to_cell_indices(seer_box.min, seen_grid->origin, seen_grid->cell_sizes, dims);
    int4 max_indices = _position_to_cell_indices(seer_box.max, seen_grid->origin, seen_grid->cell_sizes, dims);

    for (int i = 0; i < dims; ++i) {
        if (min_indices.arr[i] < 0)
            min_indices.arr[i] = 0;
        if (max_indices.arr[i] < 0)
            max_indices.arr[i] = 0;
        if (min_indices.arr[i] >= seen_grid->cell_counts.arr[i])
            min_indices.arr[i] = seen_grid->cell_counts.arr[i] - 1;
        if (max_indices.arr[i] >= seen_grid->cell_counts.arr[i])
            max_indices.arr[i] = seen_grid->cell_counts.arr[i] - 1;
    }

    AgentsSlice seen_slice;
    seen_slice.agents = pass->seen_group->storage;
    seen_slice.size = pass->seen_group->agent_size;

    int4 indices;
    switch (dims) {
        case 1: {
        } break;
        case 2: {
        } break;
        case 3: {
            for (indices.x = min_indices.x; indices.x <= max_indices.x; ++indices.x) {
                for (indices.y = min_indices.y; indices.y <= max_indices.y; ++indices.y) {
                    for (indices.z = min_indices.z; indices.z <= max_indices.z; ++indices.z) {
                        unsigned seen_cell_i = _cell_indices_to_cell_index(indices, dims);
                        ZGridCell *seen_cell = seen_grid->cells + seen_cell_i;

                        seen_slice.beg = seen_cell->first_agent_i;
                        seen_slice.end = seen_cell->first_agent_i + seen_cell->count;

                        pass->pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
                    }
                }
            }
        } break;
        default: {
        };
    }
}

static inline unsigned _min(unsigned a, unsigned b) {
    return (a < b) ? a : b;
}

static void _see_func(TayThreadTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    TayGroup *seer_group = pass->seer_group;
    TayGroup *seen_group = pass->seen_group;
    CpuZGrid *seer_grid = &seer_group->space.cpu_z_grid;

    int min_dims = (seer_group->space.dims < seen_group->space.dims) ?
                   seer_group->space.dims :
                   seen_group->space.dims;

    TayRange seers_range = tay_threads_range(pass->seer_group->space.count, task->thread_i);
    unsigned seer_i = seers_range.beg;

    while (seer_i < seers_range.end) {
        TayAgentTag *seer = (TayAgentTag *)(seer_group->storage + seer_group->agent_size * seer_i);
        ZGridCell *seer_cell = seer_grid->cells + seer->part_i;
        int4 seer_cell_indices = _cell_index_to_cell_indices(seer->part_i, min_dims);

        Box seer_box;
        for (int i = 0; i < min_dims; ++i) {
            float min = seer_grid->origin.arr[i] + seer_cell_indices.arr[i] * seer_grid->cell_sizes.arr[i];
            seer_box.min.arr[i] = min - pass->radii.arr[i];
            seer_box.max.arr[i] = min + seer_grid->cell_sizes.arr[i] + pass->radii.arr[i];
        }

        unsigned cell_end_seer_i = _min(seer_cell->first_agent_i + seer_cell->count, seers_range.end);

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

void cpu_z_grid_see(TayPass *pass) {
    space_run_thread_tasks(pass, _see_func);
}
