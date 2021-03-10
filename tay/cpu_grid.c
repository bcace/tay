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
    Bin *bins; /* for finding kernel bins */
    Bin *first_bin; /* bins are balanced into separate lists for each task */
    unsigned agents_count;
    int thread_i;
    int dims;
    unsigned modulo_mask;
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
    task->modulo_mask = grid->modulo_mask;
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
    unsigned modulo_mask = task->modulo_mask;
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
                            seen_bin = task->bins + _cell_indices_to_hash_1(seen_indices, modulo_mask);
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
                                seen_bin = task->bins + _cell_indices_to_hash_2(seen_indices, modulo_mask);
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
                                    seen_bin = task->bins + _cell_indices_to_hash_3(seen_indices, modulo_mask);
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
                                        seen_bin = task->bins + _cell_indices_to_hash_4(seen_indices, modulo_mask);
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

static void *_get_thread_mem(Space *space, int thread_i) {
    return (char *)space->shared + space->shared_size - (thread_i + 1) * space->cpu_grid.kernel_size;
}

void cpu_grid_single_space_see(Space *space, TayPass *pass) {
    static _SeeTask tasks[TAY_MAX_THREADS];
    static _SeeTask *sorted_tasks[TAY_MAX_THREADS];

    CpuGrid *grid = &space->cpu_grid;

    /* calculate kernel size */
    ushort4 kernel_radii;
    for (int i = 0; i < space->dims; ++i)
        kernel_radii.arr[i] = (int)ceilf(pass->radii.arr[i] / grid->cell_sizes.arr[i]);

    /* reset tasks */
    for (int i = 0; i < runner.count; ++i) {
        tasks[i].first_bin = 0;
        tasks[i].agents_count = 0;
        sorted_tasks[i] = tasks + i;
    }

    Bin *buckets[TAY_MAX_BUCKETS] = { 0 };

    /* sort bins into buckets wrt number of contained agents */
    for (Bin *bin = grid->first_bin; bin; bin = bin->next) {
        unsigned count = bin->counts[pass->seer_group];
        if (count) {
            int bucket_i = space_agent_count_to_bucket_index(count);
            bin->thread_next = buckets[bucket_i];
            buckets[bucket_i] = bin;
        }
    }

    /* distribute bins among threads */
    for (int bucket_i = 0; bucket_i < TAY_MAX_BUCKETS; ++bucket_i) {
        Bin *bin = buckets[bucket_i];

        while (bin) {
            Bin *next_bin = bin->thread_next;

            _SeeTask *task = sorted_tasks[0]; /* always take the task with fewest agents */
            bin->thread_next = task->first_bin;
            task->first_bin = bin;
            task->agents_count += bin->counts[pass->seer_group];

            /* sort the task wrt its number of agents */
            {
                int index = 1;
                for (; index < runner.count && task->agents_count > sorted_tasks[index]->agents_count; ++index);
                for (int i = 1; i < index; ++i)
                    sorted_tasks[i - 1] = sorted_tasks[i];
                sorted_tasks[index - 1] = task;
            }

            bin = next_bin;
        }
    }

    /* set tasks */
    for (int i = 0; i < runner.count; ++i) {
        _SeeTask *task = tasks + i;
        _init_see_task(task, pass, grid, i, space->dims, kernel_radii, _get_thread_mem(space, i));
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
            for (TayAgentTag *tag = bin->first[task->pass->act_group]; tag; tag = tag->next)
                task->pass->act(tag, thread_context->context);
        ++task->counter;
    }
}

void cpu_grid_act(Space *space, TayPass *pass) {
    static ActTask tasks[TAY_MAX_THREADS];

    CpuGrid *grid = &space->cpu_grid;

    for (int i = 0; i < runner.count; ++i) {
        ActTask *task = tasks + i;
        _init_act_task(task, pass, grid->first_bin, i);
        tay_thread_set_task(i, _act_func, task, pass->context);
    }

    tay_runner_run();
}

void cpu_grid_on_simulation_start(Space *space) {
    CpuGrid *grid = &space->cpu_grid;
    grid->bins = space->shared;
}

static unsigned _highest_power_of_two(unsigned size) {
    for (int i = 1; i < 32; ++i)
        if ((1ul << i) > size)
            return 1ul << (i - 1);
    return 0;
}

void cpu_grid_sort(Space *space, TayGroup *groups, TayPass *passes, int passes_count) {
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

    /* calculate memory needed for kernels */
    int max_kernel_bins = 0;
    for (int i = 0; i < passes_count; ++i) {
        TayPass *pass = passes + i;

        if (pass->type != TAY_PASS_SEE || groups[pass->seer_group].space != space && groups[pass->seen_group].space != space)
            continue;

        int kernel_bins = 1;
        for (int i = 0; i < space->dims; ++i)
            kernel_bins *= (int)ceilf(pass->radii.arr[i] / grid->cell_sizes.arr[i]) * 2 + 1;

        if (kernel_bins > max_kernel_bins)
            max_kernel_bins = kernel_bins;
    }

    /* calculate memory needed for bins */
    grid->kernel_size = max_kernel_bins * (int)sizeof(Bin *);
    unsigned bins_mem_size = space->shared_size - grid->kernel_size * runner.count;
    unsigned max_bins_count = bins_mem_size / (unsigned)sizeof(Bin);
    unsigned max_bins_count_fast = _highest_power_of_two(max_bins_count);
    grid->modulo_mask = max_bins_count_fast - 1;
    // ERROR: make sure there's at least one bin available

    /* sort agents into bins */
    {
        grid->first_bin = 0;

        for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            TayGroup *group = groups + group_i;
            if (group->space != space)
                continue;

            TayAgentTag *next = space->first[group_i];
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

                tag->next = bin->first[group_i];
                bin->first[group_i] = tag;
                ++bin->counts[group_i];

                if (bin->used == false) {
                    bin->next = grid->first_bin;
                    grid->first_bin = bin;
                    bin->used = true;
                }
            }

            space->first[group_i] = 0;
            space->counts[group_i] = 0;
        }
    }
}

void cpu_grid_unsort(Space *space, TayGroup *groups) {
    CpuGrid *grid = &space->cpu_grid;

    box_reset(&space->box, space->dims);

    for (Bin *bin = grid->first_bin; bin; bin = bin->next) {

        for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            TayGroup *group = groups + group_i;

            if (group->space != space) /* only groups belonging to this space */
                continue;

            space_return_agents(space, group_i, bin->first[group_i], group->is_point);

            bin->first[group_i] = 0;
            bin->counts[group_i] = 0;
        }

        bin->used = false;
    }
}
