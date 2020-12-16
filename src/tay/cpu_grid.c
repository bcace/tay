#include "space.h"
#include "thread.h"
#include <math.h>
#include <assert.h>

#define TAY_MAX_KERNELS_IN_CACHE 8


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

typedef struct Bucket {
    struct Bucket *next;
    TayAgentTag *first[TAY_MAX_GROUPS];
    int used;
    unsigned char visited[TAY_MAX_THREADS];
} Bucket;

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
    ) % TAY_MAX_CELLS;
}

static inline unsigned _cell_indices_to_hash_2(ushort4 indices) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y)
    ) % TAY_MAX_CELLS;
}

static inline unsigned _cell_indices_to_hash_3(ushort4 indices) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y) ^
        ((unsigned long long)indices.z * HASH_FACTOR_Z)
    ) % TAY_MAX_CELLS;
}

static inline unsigned _cell_indices_to_hash_4(ushort4 indices) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y) ^
        ((unsigned long long)indices.z * HASH_FACTOR_Z) ^
        ((unsigned long long)indices.w * HASH_FACTOR_W)
    ) % TAY_MAX_CELLS;
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
    Bucket *buckets;
    Bucket *first_bucket;
    int thread_i;
    int counter;
    int dims;
    ushort4 kernel_radii;
    float4 grid_origin;
    float4 cell_sizes;
    Bucket **kernel;
} _SeeTask;

static void _init_see_task(_SeeTask *task, TayPass *pass, CpuGrid *grid, int thread_i, int dims,
                           ushort4 kernel_radii, void *thread_mem) {
    task->pass = pass;
    task->buckets = grid->buckets;
    task->first_bucket = grid->first_bucket;
    task->thread_i = thread_i;
    task->counter = thread_i;
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

    for (Bucket *seer_bucket = task->first_bucket; seer_bucket; seer_bucket = seer_bucket->next) {
        if (task->counter % runner.count == 0) {

            int kernel_buckets_count = 0;
            ushort4 prev_seer_indices = { 0, 0, 0, 0 };

            for (TayAgentTag *seer_agent = seer_bucket->first[seer_group]; seer_agent; seer_agent = seer_agent->next) {
                float4 seer_p = TAY_AGENT_POSITION(seer_agent);
                ushort4 seer_indices = _agent_position_to_cell_indices(seer_p, grid_origin, cell_sizes, dims);

                if (kernel_buckets_count > 0 && ushort4_eq(prev_seer_indices, seer_indices, dims)) {
                    for (int i = 0; i < kernel_buckets_count; ++i)
                        task->kernel[i]->visited[task->thread_i] = false;
                }
                else {
                    kernel_buckets_count = 0;

                    ushort4 origin;
                    for (int i = 0; i < dims; ++i)
                        origin.arr[i] = seer_indices.arr[i] - kernel_radii.arr[i];

                    ushort4 seen_indices;
                    Bucket *seen_bucket;

                    switch (dims) {
                        case 1: {
                            for (int x = 0; x < kernel_sizes.x; ++x) {
                                seen_indices.x = origin.x + x;
                                seen_bucket = task->buckets + _cell_indices_to_hash_1(seen_indices);
                                if (seen_bucket->used) {
                                    task->kernel[kernel_buckets_count++] = seen_bucket;
                                    seen_bucket->visited[task->thread_i] = false;
                                }
                            }
                        } break;
                        case 2: {
                            for (int x = 0; x < kernel_sizes.x; ++x) {
                                seen_indices.x = origin.x + x;
                                for (int y = 0; y < kernel_sizes.y; ++y) {
                                    seen_indices.y = origin.y + y;
                                    seen_bucket = task->buckets + _cell_indices_to_hash_2(seen_indices);
                                    if (seen_bucket->used) {
                                        task->kernel[kernel_buckets_count++] = seen_bucket;
                                        seen_bucket->visited[task->thread_i] = false;
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
                                        seen_bucket = task->buckets + _cell_indices_to_hash_3(seen_indices);
                                        if (seen_bucket->used) {
                                            task->kernel[kernel_buckets_count++] = seen_bucket;
                                            seen_bucket->visited[task->thread_i] = false;
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
                                            seen_bucket = task->buckets + _cell_indices_to_hash_4(seen_indices);
                                            if (seen_bucket->used) {
                                                task->kernel[kernel_buckets_count++] = seen_bucket;
                                                seen_bucket->visited[task->thread_i] = false;
                                            }
                                        }
                                    }
                                }
                            }
                        };
                    }
                    prev_seer_indices = seer_indices;
                }

                for (int i = 0; i < kernel_buckets_count; ++i) {
                    Bucket *seen_bucket = task->kernel[i];
                    if (!seen_bucket->visited[task->thread_i]) {
                        seen_bucket->visited[task->thread_i] = true;
                        space_see_single_seer(seer_agent, seen_bucket->first[seen_group], see_func, radii, dims, thread_context);
                    }
                }
            }
        }
        ++task->counter;
    }
}

static void _see(TayState *state, int pass_index) {
    static _SeeTask tasks[TAY_MAX_THREADS];

    Space *space = &state->space;
    CpuGrid *grid = &space->cpu_grid;
    TayPass *pass = state->passes + pass_index;

    /* calculate kernel size */
    int kernel_size = 1;
    ushort4 kernel_radii;
    for (int i = 0; i < space->dims; ++i) {
        kernel_radii.arr[i] = (int)ceilf(pass->radii.arr[i] / grid->cell_sizes.arr[i]);
        kernel_size *= kernel_radii.arr[i] * 2 + 1;
    }

    assert(kernel_size * sizeof(Bucket *) <= space_get_thread_mem_size());

    for (int i = 0; i < runner.count; ++i) {
        _SeeTask *task = tasks + i;
        _init_see_task(task, pass, grid, i, space->dims, kernel_radii, space_get_thread_mem(space, i));
        tay_thread_set_task(i, _see_func, task, pass->context);
    }

    tay_runner_run();
}

typedef struct {
    TayPass *pass;
    Bucket *first_bucket;
    int counter;
} ActTask;

static void _init_act_task(ActTask *task, TayPass *pass, Bucket *first_bucket, int thread_index) {
    task->pass = pass;
    task->first_bucket = first_bucket;
    task->counter = thread_index;
}

// TODO: try with aliases...
static void _act_func(ActTask *task, TayThreadContext *thread_context) {
    for (Bucket *bucket = task->first_bucket; bucket; bucket = bucket->next) {
        if (task->counter % runner.count == 0)
            for (TayAgentTag *tag = bucket->first[task->pass->act_group]; tag; tag = tag->next)
                task->pass->act(tag, thread_context->context);
        ++task->counter;
    }
}

static void _act(TayState *state, int pass_index) {
    static ActTask tasks[TAY_MAX_THREADS];

    CpuGrid *grid = &state->space.cpu_grid;
    TayPass *pass = state->passes + pass_index;

    for (int i = 0; i < runner.count; ++i) {
        ActTask *task = tasks + i;
        _init_act_task(task, pass, grid->first_bucket, i);
        tay_thread_set_task(i, _act_func, task, pass->context);
    }

    tay_runner_run();
}

void cpu_grid_prepare(TayState *state) {
    Space *space = &state->space;
    CpuGrid *grid = &space->cpu_grid;
    grid->buckets = space_get_cell_arena(space, TAY_MAX_CELLS * sizeof(Bucket), true);
}

void cpu_grid_step(TayState *state) {
    Space *space = &state->space;
    CpuGrid *grid = &space->cpu_grid;

    /* calculate grid parameters */
    for (int i = 0; i < space->dims; ++i) {
        float cell_size = (space->radii.arr[i] * 2.0f) / (float)(1 << space->depth_correction);
        float space_size = space->box.max.arr[i] - space->box.min.arr[i];
        int count = (int)ceilf(space_size / cell_size);
        float margin = (count * cell_size - space_size) * 0.5f;
        grid->cell_counts.arr[i] = count;
        grid->cell_sizes.arr[i] = cell_size;
        grid->grid_origin.arr[i] = space->box.min.arr[i] - margin;
    }

    /* sort agents into buckets */
    {
        grid->first_bucket = 0;

        for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            TayAgentTag *next = space->first[group_i];
            while (next) {
                TayAgentTag *tag = next;
                next = next->next;

                float4 p = TAY_AGENT_POSITION(tag);
                ushort4 indices = _agent_position_to_cell_indices(p,
                                                                  grid->grid_origin,
                                                                  grid->cell_sizes,
                                                                  space->dims);
                unsigned hash = _cell_indices_to_hash(indices, space->dims);
                Bucket *bucket = grid->buckets + hash;

                tag->next = bucket->first[group_i];
                bucket->first[group_i] = tag;

                if (bucket->used == false) {
                    bucket->next = grid->first_bucket;
                    grid->first_bucket = bucket;
                    bucket->used = true;
                }
            }
            space->first[group_i] = 0;
            space->counts[group_i] = 0;
        }
    }

    /* do passes */
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            _see(state, i);
        else if (pass->type == TAY_PASS_ACT)
            _act(state, i);
        else
            assert(0); /* unhandled pass type */
    }

    /* reset the space box before returning agents */
    box_reset(&space->box, space->dims);

    /* return agents and update space box */
    for (Bucket *bucket = grid->first_bucket; bucket; bucket = bucket->next) {
        for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            space_return_agents(space, group_i, bucket->first[group_i]);
            bucket->first[group_i] = 0;
        }
        bucket->used = false;
    }
}
