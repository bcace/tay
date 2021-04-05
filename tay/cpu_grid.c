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
    struct Bin *thread_next_seer_bin;
    TayAgentTag *first;
    unsigned count;
    unsigned char visited[TAY_MAX_THREADS];
} Bin;

static inline void _clear_bin(Bin *bin) {
    bin->first = 0;
    bin->count = 0;
}

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

static inline unsigned _cell_indices_to_hash_1(ushort4 indices, unsigned modulo_mask) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X)
    ) & modulo_mask; /* this is a % operation, works with & only if done with a number that's (2^n - 1) */
}

static inline unsigned _cell_indices_to_hash_2(ushort4 indices, unsigned modulo_mask) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y)
    ) & modulo_mask; /* this is a % operation, works with & only if done with a number that's (2^n - 1) */
}

static inline unsigned _cell_indices_to_hash_3(ushort4 indices, unsigned modulo_mask) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y) ^
        ((unsigned long long)indices.z * HASH_FACTOR_Z)
    ) & modulo_mask; /* this is a % operation, works with & only if done with a number that's (2^n - 1) */
}

static inline unsigned _cell_indices_to_hash_4(ushort4 indices, unsigned modulo_mask) {
    return (
        ((unsigned long long)indices.x * HASH_FACTOR_X) ^
        ((unsigned long long)indices.y * HASH_FACTOR_Y) ^
        ((unsigned long long)indices.z * HASH_FACTOR_Z) ^
        ((unsigned long long)indices.w * HASH_FACTOR_W)
    ) & modulo_mask; /* this is a % operation, works with & only if done with a number that's (2^n - 1) */
}

static inline unsigned _cell_indices_to_hash(ushort4 indices, int dims, unsigned modulo_mask) {
    switch (dims) {
        case 1: return _cell_indices_to_hash_1(indices, modulo_mask);
        case 2: return _cell_indices_to_hash_2(indices, modulo_mask);
        case 3: return _cell_indices_to_hash_3(indices, modulo_mask);
        default: return _cell_indices_to_hash_4(indices, modulo_mask);
    }
}

typedef struct {
    TayPass *pass;
    Bin *first_thread_seer_bin; /* bins are balanced into separate lists for each task */
    int thread_i;
} _SeeTask;

static void _init_see_task(_SeeTask *task, TayPass *pass, Bin *first_thread_seer_bin, int thread_i) {
    task->pass = pass;
    task->first_thread_seer_bin = first_thread_seer_bin;
    task->thread_i = thread_i;
}

static void _see_func(_SeeTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    Space *seer_space = pass->seer_space;
    Space *seen_space = pass->seen_space;
    CpuHashGrid *seer_grid = &seer_space->cpu_grid;
    CpuHashGrid *seen_grid = &seen_space->cpu_grid;

    unsigned seen_modulo_mask = seen_grid->modulo_mask;
    int min_dims = (seer_space->dims < seen_space->dims) ? seer_space->dims : seen_space->dims;

    for (Bin *seer_bin = task->first_thread_seer_bin; seer_bin; seer_bin = seer_bin->thread_next_seer_bin) {

        Bin **kernel = 0;
        int kernel_bins_count = 0;
        ushort4 prev_seer_indices = { 0xffff, 0xffff, 0xffff, 0xffff };

        #if TAY_TELEMETRY
        if (seer_bin->first)
            ++thread_context->grid_seer_bins;
        #endif

        for (TayAgentTag *seer_agent = seer_bin->first; seer_agent; seer_agent = seer_agent->next) {
            float4 seer_p = float4_agent_position(seer_agent);
            ushort4 seer_indices = _agent_position_to_cell_indices(seer_p, seer_grid->grid_origin, seer_grid->cell_sizes, min_dims);

            if (ushort4_eq(prev_seer_indices, seer_indices, min_dims)) {
                for (int i = 0; i < kernel_bins_count; ++i)
                    kernel[i]->visited[task->thread_i] = false;
            }
            else {
                kernel_bins_count = 0;

                #if TAY_TELEMETRY
                ++thread_context->grid_seer_kernel_rebuilds;
                #endif

                unsigned max_kernel_count = 0;
                ushort4 min_seen_indices;
                ushort4 max_seen_indices;
                for (int i = 0; i < min_dims; ++i) {
                    float seer_cell_size = seer_grid->cell_sizes.arr[i];
                    float seen_cell_size = seen_grid->cell_sizes.arr[i];

                    float min = seer_grid->grid_origin.arr[i] + seer_indices.arr[i] * seer_cell_size;
                    float seer_box_min = min - pass->radii.arr[i];
                    float seer_box_max = min + seer_cell_size + pass->radii.arr[i];

                    int seen_cell_min = (int)floorf((seer_box_min - seen_grid->grid_origin.arr[i]) / seen_cell_size);
                    int seen_cell_max = (int)ceilf((seer_box_max - seen_grid->grid_origin.arr[i]) / seen_cell_size);

                    min_seen_indices.arr[i] = (unsigned short)((seen_cell_min < 0) ? 0 : seen_cell_min);
                    max_seen_indices.arr[i] = (unsigned short)((seen_cell_max < 0) ? 0 : seen_cell_max);

                    ++max_kernel_count;
                }

                kernel = tay_threads_refresh_thread_storage(thread_context, max_kernel_count * sizeof(Bin *));

                ushort4 seen_indices;
                Bin *seen_bin;

                switch (min_dims) {
                    case 1: {
                        for (seen_indices.x = min_seen_indices.x; seen_indices.x < max_seen_indices.x; ++seen_indices.x) {
                            seen_bin = seen_grid->bins + _cell_indices_to_hash_1(seen_indices, seen_modulo_mask);
                            if (seen_bin->count) {
                                kernel[kernel_bins_count++] = seen_bin;
                                seen_bin->visited[task->thread_i] = false;
                            }
                        }
                    } break;
                    case 2: {
                        for (seen_indices.x = min_seen_indices.x; seen_indices.x < max_seen_indices.x; ++seen_indices.x) {
                            for (seen_indices.y = min_seen_indices.y; seen_indices.y < max_seen_indices.y; ++seen_indices.y) {
                                seen_bin = seen_grid->bins + _cell_indices_to_hash_2(seen_indices, seen_modulo_mask);
                                if (seen_bin->count) {
                                    kernel[kernel_bins_count++] = seen_bin;
                                    seen_bin->visited[task->thread_i] = false;
                                }
                            }
                        }
                    } break;
                    case 3: {
                        for (seen_indices.x = min_seen_indices.x; seen_indices.x < max_seen_indices.x; ++seen_indices.x) {
                            for (seen_indices.y = min_seen_indices.y; seen_indices.y < max_seen_indices.y; ++seen_indices.y) {
                                for (seen_indices.z = min_seen_indices.z; seen_indices.z < max_seen_indices.z; ++seen_indices.z) {
                                    seen_bin = seen_grid->bins + _cell_indices_to_hash_3(seen_indices, seen_modulo_mask);
                                    if (seen_bin->count) {
                                        kernel[kernel_bins_count++] = seen_bin;
                                        seen_bin->visited[task->thread_i] = false;
                                    }
                                }
                            }
                        }
                    } break;
                    default: {
                        for (seen_indices.x = min_seen_indices.x; seen_indices.x < max_seen_indices.x; ++seen_indices.x) {
                            for (seen_indices.y = min_seen_indices.y; seen_indices.y < max_seen_indices.y; ++seen_indices.y) {
                                for (seen_indices.z = min_seen_indices.z; seen_indices.z < max_seen_indices.z; ++seen_indices.z) {
                                    for (seen_indices.w = min_seen_indices.w; seen_indices.w < max_seen_indices.w; ++seen_indices.w) {
                                        seen_bin = seen_grid->bins + _cell_indices_to_hash_4(seen_indices, seen_modulo_mask);
                                        if (seen_bin->count) {
                                            kernel[kernel_bins_count++] = seen_bin;
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
                Bin *seen_bin = kernel[i];
                if (!seen_bin->visited[task->thread_i]) {
                    seen_bin->visited[task->thread_i] = true;
                    pass->pairing_func(seer_agent, seen_bin->first, pass->see, pass->radii, min_dims, thread_context);
                }
            }
        }
    }
}

void cpu_grid_see(TayPass *pass) {
    static _SeeTask tasks[TAY_MAX_THREADS];

    typedef struct {
        Bin *first_thread_seer_bin;
        int agents_count;
    } SortTask;

    static SortTask sort_tasks[TAY_MAX_THREADS];

    // reset sort tasks
    for (int i = 0; i < runner.count; ++i) {
        SortTask *sort_task = sort_tasks + i;
        sort_task->first_thread_seer_bin = 0;
        sort_task->agents_count = 0;
    }

    Bin *buckets[TAY_MAX_BUCKETS] = {0};
    CpuHashGrid *seer_grid = &pass->seer_space->cpu_grid;

    /* sort bins into buckets wrt number of contained agents */
    for (Bin *bin = seer_grid->first_bin; bin; bin = bin->next) {
        unsigned count = bin->count;
        if (count) {
            int bucket_i = space_agent_count_to_bucket_index(count);
            bin->thread_next_seer_bin = buckets[bucket_i];
            buckets[bucket_i] = bin;
        }
    }

    /* distribute bins among threads */
    for (int bucket_i = 0; bucket_i < TAY_MAX_BUCKETS; ++bucket_i) {
        Bin *bin = buckets[bucket_i];

        while (bin) {
            Bin *next_bin = bin->thread_next_seer_bin;

            SortTask sort_task = sort_tasks[0]; /* always take the task with fewest agents */
            bin->thread_next_seer_bin = sort_task.first_thread_seer_bin;
            sort_task.first_thread_seer_bin = bin;
            sort_task.agents_count += bin->count;

            /* sort the task wrt its number of agents */
            {
                int index = 1;
                for (; index < runner.count && sort_task.agents_count > sort_tasks[index].agents_count; ++index);
                for (int i = 1; i < index; ++i)
                    sort_tasks[i - 1] = sort_tasks[i];
                sort_tasks[index - 1] = sort_task;
            }

            bin = next_bin;
        }
    }

    /* set tasks */
    for (int i = 0; i < runner.count; ++i) {
        SortTask *sort_task = sort_tasks + i;
        _SeeTask *task = tasks + i;
        _init_see_task(task, pass, sort_task->first_thread_seer_bin, i);
        tay_thread_set_task(i, _see_func, task, pass->context);
    }

    tay_runner_run();
}

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
            for (TayAgentTag *tag = bin->first; tag; tag = tag->next)
                task->pass->act(tag, thread_context->context);
        ++task->counter;
    }
}

void cpu_grid_act(TayPass *pass) {
    static ActTask tasks[TAY_MAX_THREADS];

    CpuHashGrid *grid = &pass->act_space->cpu_grid;

    for (int i = 0; i < runner.count; ++i) {
        ActTask *task = tasks + i;
        _init_act_task(task, pass, grid->first_bin, i);
        tay_thread_set_task(i, _act_func, task, pass->context);
    }

    tay_runner_run();
}

static unsigned _highest_power_of_two(unsigned size) {
    for (int i = 1; i < 32; ++i)
        if ((1ul << i) > size)
            return 1ul << (i - 1);
    return 0;
}

void cpu_grid_on_simulation_start(Space *space) {
    CpuHashGrid *grid = &space->cpu_grid;
    grid->bins = space->shared;
    unsigned max_bins_count_fast = _highest_power_of_two(space->shared_size / (unsigned)sizeof(Bin)); // ERROR: make sure there's at least one bin available
    grid->modulo_mask = max_bins_count_fast - 1;
    for (unsigned i = 0; i < max_bins_count_fast; ++i)
        _clear_bin(grid->bins + i);
}

void cpu_grid_sort(TayGroup *group, TayPass *passes, int passes_count) {
    Space *space = &group->space;
    CpuHashGrid *grid = &space->cpu_grid;

    /* calculate grid parameters */
    for (int i = 0; i < space->dims; ++i) {
        float cell_size = space->radii.arr[i] * 2.0f;
        float space_size = space->box.max.arr[i] - space->box.min.arr[i];
        int count = (int)ceilf(space_size / cell_size);
        float margin = (count * cell_size - space_size) * 0.5f;
        grid->cell_sizes.arr[i] = cell_size;
        grid->grid_origin.arr[i] = space->box.min.arr[i] - margin;
    }

    /* sort agents into bins */
    {
        grid->first_bin = 0;

        TayAgentTag *next = space->first;
        while (next) {
            TayAgentTag *tag = next;
            next = next->next;

            float4 p = float4_agent_position(tag);
            ushort4 indices = _agent_position_to_cell_indices(p,
                                                              grid->grid_origin,
                                                              grid->cell_sizes,
                                                              space->dims);
            unsigned hash = _cell_indices_to_hash(indices, space->dims, grid->modulo_mask);
            Bin *bin = grid->bins + hash;

            /* if this is the first time the bin was used */
            if (bin->count == 0) {
                bin->next = grid->first_bin;
                grid->first_bin = bin;
            }

            tag->next = bin->first;
            bin->first = tag;
            ++bin->count;
        }

        space->first = 0;
        space->count = 0;
    }
}

void cpu_grid_unsort(TayGroup *group) {
    Space *space = &group->space;
    CpuHashGrid *grid = &space->cpu_grid;

    box_reset(&space->box, space->dims);

    for (Bin *bin = grid->first_bin; bin; bin = bin->next) {
        space_return_agents(space, bin->first, group->is_point);
        _clear_bin(bin);
    }
}
