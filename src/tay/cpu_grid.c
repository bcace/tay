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

typedef struct GridCell {
    struct GridCell *next;
    TayAgentTag *first[TAY_MAX_GROUPS];
    int used;
} GridCell;

static inline ushort4 _agent_position_to_cell_indices(float4 pos, float4 orig, float4 size, int dims) {
    ushort4 indices;
    for (int i = 0; i < dims; ++i)
        indices.arr[i] = (unsigned short)floorf((pos.arr[i] - orig.arr[i]) / size.arr[i]);
    return indices;
}

static unsigned _cell_indices_to_hash(ushort4 indices, int dims) {
    unsigned long long h = 0;
    static unsigned long long c[] = { 1640531513ull, 2654435789ull, 2147483647ull, 6692367337ull };
    for (int i = 0; i < dims; ++i) {
        unsigned long long index = (unsigned long long)indices.arr[i];
        h ^= index * c[i];
    }
    return h % TAY_MAX_CELLS;
}

void cpu_grid_prepare(TayState *state) {
    Space *space = &state->space;
    CpuGrid *grid = &space->cpu_grid;
    grid->cells = space_get_cell_arena(space, TAY_MAX_CELLS * sizeof(GridCell), true);
}

typedef struct {
    TayPass *pass;
    GridCell *cells;
    GridCell *first_cell;
    int counter;
    int dims;
    ushort4 kernel_radii;
    float4 grid_origin;
    float4 cell_sizes;
    unsigned *kernel_cache;
    int kernels_per_thread;
} _SeeTask;

static void _init_see_task(_SeeTask *task, TayPass *pass, CpuGrid *grid, int thread_i, int dims,
                           ushort4 kernel_radii, int kernels_per_thread, int kernel_bytes, char *kernel_cache_mem) {
    task->pass = pass;
    task->cells = grid->cells;
    task->first_cell = grid->first;
    task->counter = thread_i;
    task->dims = dims;
    task->grid_origin = grid->grid_origin;
    task->cell_sizes = grid->cell_sizes;
    task->kernel_radii = kernel_radii;
    task->kernels_per_thread = kernels_per_thread;
    task->kernel_cache = (unsigned *)(kernel_cache_mem + kernels_per_thread * kernel_bytes * thread_i);
}

typedef struct {
    ushort4 indices;
    unsigned *hashes;
} KernelRef;

static void _see_func(_SeeTask *task, TayThreadContext *thread_context) {
    int seer_group = task->pass->seer_group;
    int seen_group = task->pass->seen_group;
    TAY_SEE_FUNC see_func = task->pass->see;
    float4 radii = task->pass->radii;
    int dims = task->dims;
    ushort4 kernel_radii = task->kernel_radii;
    float4 grid_origin = task->grid_origin;
    float4 cell_sizes = task->cell_sizes;

    int kernel_hashes_count = 1;
    int4 kernel_sizes;
    for (int i = 0; i < dims; ++i) {
        kernel_sizes.arr[i] = kernel_radii.arr[i] * 2 + 1;
        kernel_hashes_count *= kernel_sizes.arr[i];
    }

    static KernelRef kernels[TAY_MAX_KERNELS_IN_CACHE];
    for (int i = 0; i < task->kernels_per_thread; ++i)
        kernels[i].hashes = task->kernel_cache + kernel_hashes_count * i;

    for (GridCell *seer_cell = task->first_cell; seer_cell; seer_cell = seer_cell->next) {
        if (task->counter % runner.count == 0) {

            int kernels_count = 0;

            for (TayAgentTag *seer_agent = seer_cell->first[seer_group]; seer_agent; seer_agent = seer_agent->next) {
                float4 seer_p = TAY_AGENT_POSITION(seer_agent);
                ushort4 seer_indices = _agent_position_to_cell_indices(seer_p, grid_origin, cell_sizes, dims);

                /* see if this agent has a kernel in the cache */
                int kernel_i = 0;
                for (; kernel_i < kernels_count; ++kernel_i)
                    if (ushort4_eq(kernels[kernel_i].indices, seer_indices, dims))
                        break;

                /* if not found create it and move it to front */
                if (kernel_i == kernels_count) {
                    if (kernel_i == task->kernels_per_thread)
                        --kernel_i;
                    else
                        kernel_i = kernels_count++;

                    KernelRef *kernel_ref = kernels + kernel_i;
                    kernel_ref->indices = seer_indices;

                    ushort4 origin;
                    for (int i = 0; i < dims; ++i)
                        origin.arr[i] = seer_indices.arr[i] - kernel_radii.arr[i];

                    int i = 0;
                    ushort4 seen_indices;
                    if (dims == 3) {
                        for (int x = 0; x < kernel_sizes.x; ++x) {
                            seen_indices.x = origin.x + x;
                            for (int y = 0; y < kernel_sizes.y; ++y) {
                                seen_indices.y = origin.y + y;
                                for (int z = 0; z < kernel_sizes.z; ++z) {
                                    seen_indices.z = origin.z + z;
                                    kernel_ref->hashes[i++] = _cell_indices_to_hash(seen_indices, dims);
                                }
                            }
                        }
                    }
                }

                /* move currently used kernel to front */
                {
                    KernelRef tmp = kernels[kernel_i];
                    for (int i = kernel_i; i > 0; --i)
                        kernels[i] = kernels[i - 1];
                    kernels[0] = tmp;
                }

                /* use the front kernel */
                KernelRef *kernel = kernels;
                for (int i = 0; i < kernel_hashes_count; ++i) {
                    GridCell *seen_cell = task->cells + kernel->hashes[i];

                    if (seen_cell->used) {
                        for (TayAgentTag *seen_agent = seen_cell->first[seen_group]; seen_agent; seen_agent = seen_agent->next) {
                            float4 seen_p = TAY_AGENT_POSITION(seen_agent);

                            if (seer_agent == seen_agent)
                                continue;

                            for (int j = 0; j < dims; ++j) {
                                float d = seer_p.arr[j] - seen_p.arr[j];
                                if (d < -radii.arr[j] || d > radii.arr[j])
                                    goto SKIP_SEE;
                            }

                            see_func(seer_agent, seen_agent, thread_context->context);

                            SKIP_SEE:;
                        }
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
    int kernel_count = 1;
    ushort4 kernel_radii;
    for (int i = 0; i < space->dims; ++i) {
        kernel_radii.arr[i] = (int)ceilf(pass->radii.arr[i] / grid->cell_sizes.arr[i]);
        kernel_count += kernel_radii.arr[i] * 2 + 1;
    }

    /* get memory for per-thread kernel caches */
    int bytes_per_thread = (int)floorf(TAY_CPU_SHARED_MISC_ARENA_SIZE / (float)runner.count);
    int kernel_bytes = (int)(kernel_count * sizeof(unsigned));
    int kernels_per_thread = (int)floorf(bytes_per_thread / (float)kernel_bytes);
    if (kernels_per_thread > TAY_MAX_KERNELS_IN_CACHE)
        kernels_per_thread = TAY_MAX_KERNELS_IN_CACHE;
    assert(kernels_per_thread > 0);

    char *kernel_cache_mem = space_get_misc_arena(space, TAY_CPU_SHARED_MISC_ARENA_SIZE, false);

    for (int i = 0; i < runner.count; ++i) {
        _SeeTask *task = tasks + i;
        _init_see_task(task, pass, grid, i, space->dims,
                       kernel_radii, kernels_per_thread, kernel_bytes, kernel_cache_mem);
        tay_thread_set_task(i, _see_func, task, pass->context);
    }

    tay_runner_run();
}

typedef struct {
    TayPass *pass;
    GridCell *first_cell;
    int counter;
} ActTask;

static void _init_act_task(ActTask *task, TayPass *pass, GridCell *first_cell, int thread_index) {
    task->pass = pass;
    task->first_cell = first_cell;
    task->counter = thread_index;
}

// TODO: try with aliases...
static void _act_func(ActTask *task, TayThreadContext *thread_context) {
    for (GridCell *cell = task->first_cell; cell; cell = cell->next) {
        if (task->counter % runner.count == 0)
            for (TayAgentTag *tag = cell->first[task->pass->act_group]; tag; tag = tag->next)
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
        _init_act_task(task, pass, grid->first, i);
        tay_thread_set_task(i, _act_func, task, pass->context);
    }

    tay_runner_run();
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

    int collisions = 0;

    /* sort agents into cells */
    {
        grid->first = 0;

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
                GridCell *cell = grid->cells + hash;

                tag->next = cell->first[group_i];
                cell->first[group_i] = tag;

                if (cell->used == false) {
                    cell->next = grid->first;
                    grid->first = cell;
                    // cell->indices = indices;
                    cell->used = true;
                }
                // else {
                //     if (cell->indices.x != indices.x ||
                //         cell->indices.y != indices.y ||
                //         cell->indices.z != indices.z)
                //         ++collisions;
                // }
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
    for (GridCell *cell = grid->first; cell; cell = cell->next) {
        for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            space_return_agents(space, group_i, cell->first[group_i]);
            cell->first[group_i] = 0;
        }
        cell->used = false;
    }
}
