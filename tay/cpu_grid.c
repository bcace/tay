#include "space.h"
#include "thread.h"
#include <math.h>
#include <assert.h>


typedef struct {
    union {
        struct {
            unsigned short x, y, z, w;
        };
        unsigned short arr[4];
    };
} ushort4;

static int ushort4_eq(ushort4 a, ushort4 b, int dims) {
    for (int i = 0; i < dims; ++i)
        if (a.arr[i] != b.arr[i])
            return false;
    return true;
}

typedef struct Bin {
    struct Bin *next;
    struct Bin *thread_next;
    TayAgentTag *first[TAY_MAX_GROUPS];
    unsigned counts[TAY_MAX_GROUPS];
    int used;
    unsigned char visited[TAY_MAX_THREADS];
} Bin;

static inline ushort4 _agent_position_to_cell_indices(float4 pos, float4 orig, float4 size, int dims) {
    ushort4 indices;
    for (int i = 0; i < dims; ++i)
        indices.arr[i] = (unsigned short)floorf((pos.arr[i] - orig.arr[i]) / size.arr[i]);
    return indices;
}

#define HASH_FACTOR_X 1640531513ull
#define HASH_FACTOR_Y 2654435789ull
#define HASH_FACTOR_Z 2147483647ull
#define HASH_FACTOR_W 6692367337ull

static inline unsigned _cell_indices_to_hash_1(ushort4 indices) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X)
    ) & TAY_MAX_CELLS; /* this is a % operation, works with & only if done with a number that's (2^n - 1) */
}

static inline unsigned _cell_indices_to_hash_2(ushort4 indices) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y)
    ) & TAY_MAX_CELLS; /* this is a % operation, works with & only if done with a number that's (2^n - 1) */
}

static inline unsigned _cell_indices_to_hash_3(ushort4 indices) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y) ^
        ((unsigned long long)indices.z * HASH_FACTOR_Z)
    ) & TAY_MAX_CELLS; /* this is a % operation, works with & only if done with a number that's (2^n - 1) */
}

static inline unsigned _cell_indices_to_hash_4(ushort4 indices) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y) ^
        ((unsigned long long)indices.z * HASH_FACTOR_Z) ^
        ((unsigned long long)indices.w * HASH_FACTOR_W)
    ) & TAY_MAX_CELLS; /* this is a % operation, works with & only if done with a number that's (2^n - 1) */
}

static inline unsigned _cell_indices_to_hash(ushort4 indices, int dims) {
    switch (dims) {
        case 1: return _cell_indices_to_hash_1(indices);
        case 2: return _cell_indices_to_hash_2(indices);
        case 3: return _cell_indices_to_hash_3(indices);
        default: return _cell_indices_to_hash_4(indices);
    }
}

typedef struct {
    TayPass *pass;
    Bin *bins;          /* for finding kernel bins */
    Bin *first_bin;     /* bins are balanced into separate lists for each task */
    unsigned agents_count;
    int thread_i;
    int dims;
    ushort4 kernel_radii;
    float4 grid_origin;
    float4 cell_sizes;
    Bin **kernel;
} _SeeTask;

static void _init_see_task(_SeeTask *task, TayPass *pass, CpuGrid *grid, int thread_i, int dims,
                           ushort4 kernel_radii, void *thread_mem) {
    task->pass = pass;
    task->bins = grid->bins;
    task->thread_i = thread_i;
    task->dims = dims;
    task->grid_origin = grid->grid_origin;
    task->cell_sizes = grid->cell_sizes;
    task->kernel_radii = kernel_radii;
    task->kernel = thread_mem;
}

static void _see_func(_SeeTask *task, TayThreadContext *thread_context) {
    int seer_group = task->pass->seer_group;
    int seen_group = task->pass->seen_group;
    TAY_SEE_FUNC see_func = task->pass->see;
    float4 radii = task->pass->radii;
    int dims = task->dims;
    ushort4 kernel_radii = task->kernel_radii;
    float4 grid_origin = task->grid_origin;
    float4 cell_sizes = task->cell_sizes;

    int4 kernel_sizes;
    for (int i = 0; i < dims; ++i)
        kernel_sizes.arr[i] = kernel_radii.arr[i] * 2 + 1;

    for (Bin *seer_bin = task->first_bin; seer_bin; seer_bin = seer_bin->thread_next) {
        int kernel_bins_count = 0;
        ushort4 prev_seer_indices = { 0xffff, 0xffff, 0xffff, 0xffff };

#if TAY_TELEMETRY
        if (seer_bin->first[seer_group])
            ++thread_context->grid_seer_bins;
#endif

        for (TayAgentTag *seer_agent = seer_bin->first[seer_group]; seer_agent; seer_agent = seer_agent->next) {
            float4 seer_p = float4_agent_position(seer_agent);
            ushort4 seer_indices = _agent_position_to_cell_indices(seer_p, grid_origin, cell_sizes, dims);

            if (ushort4_eq(prev_seer_indices, seer_indices, dims)) {
                for (int i = 0; i < kernel_bins_count; ++i)
                    task->kernel[i]->visited[task->thread_i] = false;
            }
            else {
                kernel_bins_count = 0;

#if TAY_TELEMETRY
                ++thread_context->grid_seer_kernel_rebuilds;
#endif

                ushort4 origin;
                for (int i = 0; i < dims; ++i)
                    origin.arr[i] = seer_indices.arr[i] - kernel_radii.arr[i];

                ushort4 seen_indices;
                Bin *seen_bin;

                switch (dims) {
                    case 1: {
                        for (int x = 0; x < kernel_sizes.x; ++x) {
                            seen_indices.x = origin.x + x;
                            seen_bin = task->bins + _cell_indices_to_hash_1(seen_indices);
                            if (seen_bin->counts[seer_group]) {
                                task->kernel[kernel_bins_count++] = seen_bin;
                                seen_bin->visited[task->thread_i] = false;
                            }
                        }
                    } break;
                    case 2: {
                        for (int x = 0; x < kernel_sizes.x; ++x) {
                            seen_indices.x = origin.x + x;
                            for (int y = 0; y < kernel_sizes.y; ++y) {
                                seen_indices.y = origin.y + y;
                                seen_bin = task->bins + _cell_indices_to_hash_2(seen_indices);
                                if (seen_bin->counts[seer_group]) {
                                    task->kernel[kernel_bins_count++] = seen_bin;
                                    seen_bin->visited[task->thread_i] = false;
                                }
                            }
                        }
                    } break;
                    case 3: {
                        for (int x = 0; x < kernel_sizes.x; ++x) {
                            seen_indices.x = origin.x + x;
                            for (int y = 0; y < kernel_sizes.y; ++y) {
                                seen_indices.y = origin.y + y;
                                for (int z = 0; z < kernel_sizes.z; ++z) {
                                    seen_indices.z = origin.z + z;
                                    seen_bin = task->bins + _cell_indices_to_hash_3(seen_indices);
                                    if (seen_bin->counts[seer_group]) {
                                        task->kernel[kernel_bins_count++] = seen_bin;
                                        seen_bin->visited[task->thread_i] = false;
                                    }
                                }
                            }
                        }
                    } break;
                    default: {
                        for (int x = 0; x < kernel_sizes.x; ++x) {
                            seen_indices.x = origin.x + x;
                            for (int y = 0; y < kernel_sizes.y; ++y) {
                                seen_indices.y = origin.y + y;
                                for (int z = 0; z < kernel_sizes.z; ++z) {
                                    seen_indices.z = origin.z + z;
                                    for (int w = 0; w < kernel_sizes.w; ++w) {
                                        seen_indices.w = origin.w + w;
                                        seen_bin = task->bins + _cell_indices_to_hash_4(seen_indices);
                                        if (seen_bin->counts[seer_group]) {
                                            task->kernel[kernel_bins_count++] = seen_bin;
                                            seen_bin->visited[task->thread_i] = false;
                                        }
                                    }
                                }
                            }
                        }
                    };
                }
                prev_seer_indices = seer_indices;
            }

            for (int i = 0; i < kernel_bins_count; ++i) {
                Bin *seen_bin = task->kernel[i];
                if (!seen_bin->visited[task->thread_i]) {
                    seen_bin->visited[task->thread_i] = true;
                    space_single_seer_see(seer_agent, seen_bin->first[seen_group], see_func, radii, dims, thread_context);
                }
            }
        }
    }
}

// static void _see(TayState *state, int pass_index) {
//     static _SeeTask tasks[TAY_MAX_THREADS];
//     static _SeeTask *sorted_tasks[TAY_MAX_THREADS];

//     Space *space = &state->space;
//     CpuGrid *grid = &space->cpu_grid;
//     TayPass *pass = state->passes + pass_index;

//     /* calculate kernel size */
//     int kernel_size = 1;
//     ushort4 kernel_radii;
//     for (int i = 0; i < space->dims; ++i) {
//         kernel_radii.arr[i] = (int)ceilf(pass->radii.arr[i] / grid->cell_sizes.arr[i]);
//         kernel_size *= kernel_radii.arr[i] * 2 + 1;
//     }
//     int a = space_get_thread_mem_size();
//     int b = kernel_size * sizeof(Bin *);
//     int c = a / b;
//     assert(kernel_size * sizeof(Bin *) <= space_get_thread_mem_size());

//     /* reset tasks */
//     for (int i = 0; i < runner.count; ++i) {
//         tasks[i].first_bin = 0;
//         tasks[i].agents_count = 0;
//         sorted_tasks[i] = tasks + i;
//     }

//     Bin *buckets[TAY_MAX_BUCKETS] = { 0 };

//     /* sort bins into buckets wrt number of contained agents */
//     for (Bin *bin = grid->first_bin; bin; bin = bin->next) {
//         unsigned count = bin->counts[pass->seer_group];
//         if (count) {
//             int bucket_i = space_agent_count_to_bucket_index(count);
//             bin->thread_next = buckets[bucket_i];
//             buckets[bucket_i] = bin;
//         }
//     }

//     /* distribute bins among threads */
//     for (int bucket_i = 0; bucket_i < TAY_MAX_BUCKETS; ++bucket_i) {
//         Bin *bin = buckets[bucket_i];

//         while (bin) {
//             Bin *next_bin = bin->thread_next;

//             _SeeTask *task = sorted_tasks[0]; /* always take the task with fewest agents */
//             bin->thread_next = task->first_bin;
//             task->first_bin = bin;
//             task->agents_count += bin->counts[pass->seer_group];

//             /* sort the task wrt its number of agents */
//             {
//                 int index = 1;
//                 for (; index < runner.count && task->agents_count > sorted_tasks[index]->agents_count; ++index);
//                 for (int i = 1; i < index; ++i)
//                     sorted_tasks[i - 1] = sorted_tasks[i];
//                 sorted_tasks[index - 1] = task;
//             }

//             bin = next_bin;
//         }
//     }

//     /* set tasks */
//     for (int i = 0; i < runner.count; ++i) {
//         _SeeTask *task = tasks + i;
//         _init_see_task(task, pass, grid, i, space->dims, kernel_radii, space_get_thread_mem(space, i));
//         tay_thread_set_task(i, _see_func, task, pass->context);
//     }

//     tay_runner_run();
// }

typedef struct {
    TayPass *pass;
    Bin *first_bin;
    int counter;
} ActTask;

static void _init_act_task(ActTask *task, TayPass *pass, Bin *first_bin, int thread_index) {
    task->pass = pass;
    task->first_bin = first_bin;
    task->counter = thread_index;
}

// TODO: try with aliases...
static void _act_func(ActTask *task, TayThreadContext *thread_context) {
    for (Bin *bin = task->first_bin; bin; bin = bin->next) {
        if (task->counter % runner.count == 0)
            for (TayAgentTag *tag = bin->first[task->pass->act_group]; tag; tag = tag->next)
                task->pass->act(tag, thread_context->context);
        ++task->counter;
    }
}

// static void _act(TayState *state, int pass_index) {
//     static ActTask tasks[TAY_MAX_THREADS];

//     CpuGrid *grid = &state->space.cpu_grid;
//     TayPass *pass = state->passes + pass_index;

//     for (int i = 0; i < runner.count; ++i) {
//         ActTask *task = tasks + i;
//         _init_act_task(task, pass, grid->first_bin, i);
//         tay_thread_set_task(i, _act_func, task, pass->context);
//     }

//     tay_runner_run();
// }

void cpu_grid_on_type_switch(Space *space) {
    CpuGrid *grid = &space->cpu_grid;
    grid->bins = space_get_cell_arena(space, TAY_MAX_CELLS * sizeof(Bin), true);
}

// void cpu_grid_step(TayState *state) {
//     Space *space = &state->space;
//     CpuGrid *grid = &space->cpu_grid;

//     /* calculate grid parameters */
//     for (int i = 0; i < space->dims; ++i) {
//         float cell_size = (space->radii.arr[i] * 2.0f) / (float)(1 << space->depth_correction);
//         float space_size = space->box.max.arr[i] - space->box.min.arr[i];
//         int count = (int)ceilf(space_size / cell_size);
//         float margin = (count * cell_size - space_size) * 0.5f;
//         grid->cell_counts.arr[i] = count;
//         grid->cell_sizes.arr[i] = cell_size;
//         grid->grid_origin.arr[i] = space->box.min.arr[i] - margin;
//     }

//     /* sort agents into bins */
//     {
//         grid->first_bin = 0;

//         for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {

//             TayAgentTag *next = space->first[group_i];
//             while (next) {
//                 TayAgentTag *tag = next;
//                 next = next->next;

//                 float4 p = float4_agent_position(tag);
//                 ushort4 indices = _agent_position_to_cell_indices(p,
//                                                                   grid->grid_origin,
//                                                                   grid->cell_sizes,
//                                                                   space->dims);
//                 unsigned hash = _cell_indices_to_hash(indices, space->dims);
//                 Bin *bin = grid->bins + hash;

//                 tag->next = bin->first[group_i];
//                 bin->first[group_i] = tag;
//                 ++bin->counts[group_i];

//                 if (bin->used == false) {
//                     bin->next = grid->first_bin;
//                     grid->first_bin = bin;
//                     bin->used = true;
//                 }
//             }

//             space->first[group_i] = 0;
//             space->counts[group_i] = 0;
//         }
//     }

//     /* do passes */
//     // for (int i = 0; i < state->passes_count; ++i) {
//     //     TayPass *pass = state->passes + i;
//     //     if (pass->type == TAY_PASS_SEE)
//     //         _see(state, i);
//     //     else if (pass->type == TAY_PASS_ACT)
//     //         _act(state, i);
//     //     else
//     //         assert(0); /* unhandled pass type */
//     // }

//     /* reset the space box before returning agents */
//     box_reset(&space->box, space->dims);

//     /* return agents and update space box */
//     for (Bin *bin = grid->first_bin; bin; bin = bin->next) {
//         for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
//             space_return_agents(space, group_i, bin->first[group_i]);
//             bin->first[group_i] = 0;
//             bin->counts[group_i] = 0;
//         }
//         bin->used = false;
//     }
// }
