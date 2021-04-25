#include "space.h"
#include "thread.h"
#include <math.h>
#include <string.h>


typedef struct GridCell {
    struct GridCell *next; /* next full cell */
    unsigned count; /* agents count */
    unsigned first_agent_i;
    int4 indices;
} GridCell;

typedef struct {
    unsigned cell_i;
    unsigned cell_agent_i; /* index of agent in cell */
} Tag;

static inline void _clear_cell(GridCell *cell) {
    cell->count = 0;
}

static inline int4 _agent_position_to_cell_indices(float4 pos, float4 orig, float4 sizes, int dims) {
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

static inline unsigned _cell_indices_to_cell_index(int4 indices, int4 counts, int dims) {
    switch (dims) {
        case 1: {
            return indices.x;
        } break;
        case 2: {
            return indices.y * counts.x +
                   indices.x;
        } break;
        case 3: {
            return indices.z * counts.x * counts.y +
                   indices.y * counts.x +
                   indices.x;
        } break;
        default: {
            return indices.w * counts.x * counts.y * counts.z +
                   indices.z * counts.x * counts.y +
                   indices.y * counts.x +
                   indices.x;
        };
    }
}

void cpu_grid_on_simulation_start(Space *space) {
    CpuGrid *grid = &space->cpu_grid;
    grid->cells = space->shared;
    grid->max_cells = space->shared_size / (int)sizeof(GridCell);
    for (int i = 0; i < grid->max_cells; ++i)
        _clear_cell(grid->cells + i);
}

void cpu_grid_sort(TayGroup *group) {
    Space *space = &group->space;
    CpuGrid *grid = &space->cpu_grid;

    space_update_box(group);

    grid->first_cell = 0;
    grid->origin = space->box.min;

    unsigned long long cells_count = 1;
    int *cell_counts = grid->cell_counts.arr;
    float *cell_sizes = grid->cell_sizes.arr;

    /* calculate initial cell sizes (smallest without going below suggested smallest partition sizes) */
    for (int i = 0; i < space->dims; ++i) {
        float cell_size = space->radii.arr[i] * 2.0f;
        float space_size = space->box.max.arr[i] - space->box.min.arr[i] + cell_size * 0.001f;

        cell_counts[i] = (int)floorf(space_size / cell_size);
        cell_sizes[i] = space_size / (float)cell_counts[i];

        cells_count *= cell_counts[i];
    }

    /* if there's not enough memory for all those cells go for smallest cells that will still fit */
    if (cells_count > grid->max_cells) {
        float volume_ratio = cells_count / (float)grid->max_cells;
        float ratio = powf(volume_ratio, 1.0f / (float)space->dims);

        for (int i = 0; i < space->dims; ++i) {
            float cell_size = space->radii.arr[i] * 2.0f;
            float space_size = space->box.max.arr[i] - space->box.min.arr[i] + cell_size * 0.001f;

            cell_counts[i] = (int)floorf(cell_counts[i] / ratio);
            cell_sizes[i] = space_size / (float)cell_counts[i];
        }
    }

    /* find cells and agent indices in those cells */

    GridCell *last_cell = 0;

    for (unsigned i = 0; i < space->count; ++i) {
        Tag *agent = (Tag *)((char *)group->storage + group->agent_size * i);

        float4 p = float4_agent_position(agent);

        int4 indices = _agent_position_to_cell_indices(p, grid->origin, grid->cell_sizes, space->dims);
        unsigned cell_i = _cell_indices_to_cell_index(indices, grid->cell_counts, space->dims);

        GridCell *cell = grid->cells + cell_i;

        if (cell->count == 0) { /* if first agent in this cell, add the cell to the list of non-empty cells */
            cell->next = 0; //grid->first_cell;
            if (last_cell)
                last_cell->next = cell;
            else
                grid->first_cell = cell;
            last_cell = cell;
            cell->indices = indices;
        }

        agent->cell_i = cell_i;
        agent->cell_agent_i = cell->count;

        ++cell->count;
    }

    unsigned first_agent_i = 0;
    for (GridCell *cell = grid->first_cell; cell; cell = cell->next) {
        cell->first_agent_i = first_agent_i;
        first_agent_i += cell->count;
    }

    for (unsigned i = 0; i < space->count; ++i) {
        Tag *src = (Tag *)((char *)group->storage + group->agent_size * i);
        unsigned sorted_agent_i = grid->cells[src->cell_i].first_agent_i + src->cell_agent_i;
        Tag *dst = (Tag *)((char *)group->seen_storage + group->agent_size * sorted_agent_i);
        memcpy(dst, src, group->agent_size);
    }

    void *storage = group->storage;
    group->storage = group->seen_storage;
    group->seen_storage = storage;
}

void cpu_grid_unsort(TayGroup *group) {
    Space *space = &group->space;
    CpuGrid *grid = &space->cpu_grid;

    for (GridCell *cell = grid->first_cell; cell; cell = cell->next)
        _clear_cell(cell);
}

typedef struct {
    TayPass *pass;
    int thread_i;
} GridActTask;

static void _init_act_task(GridActTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->thread_i = thread_i;
}

static void _act_func(GridActTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;

    int agents_per_thread = pass->act_group->space.count / runner.count;
    unsigned beg_agent_i = agents_per_thread * task->thread_i;
    unsigned end_agent_i = (task->thread_i < runner.count - 1) ?
                           beg_agent_i + agents_per_thread :
                           pass->act_group->space.count;

    for (unsigned agent_i = beg_agent_i; agent_i < end_agent_i; ++agent_i) {
        void *agent = (char *)pass->act_group->storage + pass->act_group->agent_size * agent_i;
        pass->act(agent, thread_context->context);
    }
}

void cpu_grid_act(TayPass *pass) {
    static GridActTask tasks[TAY_MAX_THREADS];

    for (int thread_i = 0; thread_i < runner.count; ++thread_i) {
        GridActTask *task = tasks + thread_i;
        _init_act_task(task, pass, thread_i);
        tay_thread_set_task(thread_i, _act_func, task, pass->context);
    }

    tay_runner_run();
}

typedef struct {
    TayPass *pass;
    int thread_i;
} GridSeeTask;

static void _init_see_task(GridSeeTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->thread_i = thread_i;
}

void cpu_grid_see_seen_new(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, TayThreadContext *thread_context) {
    CpuGrid *seen_grid = &pass->seen_space->cpu_grid;

    int4 min_indices = _agent_position_to_cell_indices(seer_box.min, seen_grid->origin, seen_grid->cell_sizes, dims);
    int4 max_indices = _agent_position_to_cell_indices(seer_box.max, seen_grid->origin, seen_grid->cell_sizes, dims);

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
    seen_slice.agents = (pass->seer_group == pass->seen_group) ? pass->seen_group->seen_storage : pass->seen_group->storage;
    seen_slice.size = pass->seen_group->agent_size;

    int4 indices;
    switch (dims) {
        case 1: {
            for (indices.x = min_indices.x; indices.x <= max_indices.x; ++indices.x) {
                unsigned seen_cell_i = _cell_indices_to_cell_index(indices, seen_grid->cell_counts, dims);
                GridCell *seen_cell = seen_grid->cells + seen_cell_i;
                
                seen_slice.beg = seen_cell->first_agent_i;
                seen_slice.end = seen_cell->first_agent_i + seen_cell->count;

                pass->new_pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
            }
        } break;
        case 2: {
            for (indices.x = min_indices.x; indices.x <= max_indices.x; ++indices.x) {
                for (indices.y = min_indices.y; indices.y <= max_indices.y; ++indices.y) {
                    unsigned seen_cell_i = _cell_indices_to_cell_index(indices, seen_grid->cell_counts, dims);
                    GridCell *seen_cell = seen_grid->cells + seen_cell_i;
                    
                    seen_slice.beg = seen_cell->first_agent_i;
                    seen_slice.end = seen_cell->first_agent_i + seen_cell->count;

                    pass->new_pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
                }
            }
        } break;
        case 3: {
            for (indices.x = min_indices.x; indices.x <= max_indices.x; ++indices.x) {
                for (indices.y = min_indices.y; indices.y <= max_indices.y; ++indices.y) {
                    for (indices.z = min_indices.z; indices.z <= max_indices.z; ++indices.z) {
                        unsigned seen_cell_i = _cell_indices_to_cell_index(indices, seen_grid->cell_counts, dims);
                        GridCell *seen_cell = seen_grid->cells + seen_cell_i;

                        seen_slice.beg = seen_cell->first_agent_i;
                        seen_slice.end = seen_cell->first_agent_i + seen_cell->count;

                        pass->new_pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
                    }
                }
            }
        } break;
        default: {
            for (indices.x = min_indices.x; indices.x <= max_indices.x; ++indices.x) {
                for (indices.y = min_indices.y; indices.y <= max_indices.y; ++indices.y) {
                    for (indices.z = min_indices.z; indices.z <= max_indices.z; ++indices.z) {
                        for (indices.w = min_indices.w; indices.w <= max_indices.w; ++indices.w) {
                            unsigned seen_cell_i = _cell_indices_to_cell_index(indices, seen_grid->cell_counts, dims);
                            GridCell *seen_cell = seen_grid->cells + seen_cell_i;
                            
                            seen_slice.beg = seen_cell->first_agent_i;
                            seen_slice.end = seen_cell->first_agent_i + seen_cell->count;

                            pass->new_pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
                        }
                    }
                }
            }
        };
    }
}

static inline unsigned _min(unsigned a, unsigned b) {
    return (a < b) ? a : b;
}

static void _see_func(GridSeeTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    TayGroup *seer_group = pass->seer_group;
    TayGroup *seen_group = pass->seen_group;
    CpuGrid *seer_grid = &seer_group->space.cpu_grid;
    CpuGrid *seen_grid = &seen_group->space.cpu_grid;

    int min_dims = (seer_group->space.dims < seen_group->space.dims) ?
                   seer_group->space.dims :
                   seen_group->space.dims;

    int seers_per_thread = pass->seer_group->space.count / runner.count;
    unsigned beg_seer_i = seers_per_thread * task->thread_i;
    unsigned end_seer_i = (task->thread_i < runner.count - 1) ?
                          beg_seer_i + seers_per_thread :
                          pass->seer_group->space.count;

    Box seer_box;
    unsigned seer_i = beg_seer_i;

    while (seer_i < end_seer_i) {
        Tag *seer = (Tag *)((char *)seer_group->storage + seer_group->agent_size * seer_i);
        GridCell *seer_cell = seer_grid->cells + seer->cell_i;

        for (int i = 0; i < min_dims; ++i) {
            float min = seer_grid->origin.arr[i] + seer_cell->indices.arr[i] * seer_grid->cell_sizes.arr[i];
            seer_box.min.arr[i] = min - pass->radii.arr[i];
            seer_box.max.arr[i] = min + seer_grid->cell_sizes.arr[i] + pass->radii.arr[i];
        }

        unsigned cell_end_seer_i = _min(seer_cell->first_agent_i + seer_cell->count, end_seer_i);
        
        AgentsSlice seer_slice = {
            seer_group->storage,
            seer_group->agent_size,
            seer_i,
            cell_end_seer_i,
        };
        
        pass->new_seen_func(pass, seer_slice, seer_box, min_dims, thread_context);

        seer_i = cell_end_seer_i;
    }
}

void cpu_grid_see(TayPass *pass) {

    if (pass->seer_group == pass->seen_group) {
        TayGroup *seen_group = pass->seen_group;
        memcpy(seen_group->seen_storage, seen_group->storage, seen_group->agent_size * seen_group->space.count);
    }

    static GridSeeTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        GridSeeTask *task = tasks + i;
        _init_see_task(task, pass, i);
        tay_thread_set_task(i, _see_func, task, pass->context);
    }

    tay_runner_run();
}
