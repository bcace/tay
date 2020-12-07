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

typedef struct GridCell {
    struct GridCell *next;
    TayAgentTag *first[TAY_MAX_GROUPS];
    int used;
    ushort4 indices;
} GridCell;

static inline int _agent_position_to_cell_index(float p, float o, float s) {
    return (int)floorf((p - o) / s);
}

static inline ushort4 _agent_position_to_cell_indices(float4 pos, float4 orig, float4 size, int dims) {
    ushort4 indices;
    for (int i = 0; i < dims; ++i)
        indices.arr[i] = (unsigned short)floorf((pos.arr[i] - orig.arr[i]) / size.arr[i]);
    return indices;
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

static int _cell_indices_to_hash(ushort4 indices, int dims) {
    unsigned long long h = 0;
    static unsigned long long c[] = { 1640531513, 2654435789, 2147483647, 6692367337 };
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
    int4 kernel_radii;
} SeeTask;

static void _init_see_task(SeeTask *task, TayPass *pass, GridCell *cells, GridCell *first_cell, int thread_index, int dims, int4 kernel_radii) {
    task->pass = pass;
    task->cells = cells;
    task->first_cell = first_cell;
    task->counter = thread_index;
    task->dims = dims;
    task->kernel_radii = kernel_radii;
}

static void _see_func(SeeTask *task, TayThreadContext *thread_context) {
    int seer_group = task->pass->seer_group;
    int seen_group = task->pass->seen_group;
    TAY_SEE_FUNC see_func = task->pass->see;
    float4 radii = task->pass->radii;
    int dims = task->dims;
    int4 kernel_radii = task->kernel_radii;

    int4 kernel_sizes;
    for (int i = 0; i < dims; ++i)
        kernel_sizes.arr[i] = kernel_radii.arr[i] * 2 + 1;

    for (GridCell *seer_cell = task->first_cell; seer_cell; seer_cell = seer_cell->next) {
        if (task->counter % runner.count == 0) {

            /* find the kernel's origin cell */
            ushort4 origin;
            for (int i = 0; i < dims; ++i)
                origin.arr[i] = seer_cell->indices.arr[i] - kernel_radii.arr[i];

            ushort4 indices;
            if (dims == 3) {

                for (int x = 0; x < kernel_sizes.x; ++x) {
                    indices.x = origin.x + x;
                    for (int y = 0; y < kernel_sizes.y; ++y) {
                        indices.y = origin.y + y;
                        for (int z = 0; z < kernel_sizes.z; ++z) {
                            indices.z = origin.z + z;

                            unsigned short hash = _cell_indices_to_hash(indices, dims);
                            GridCell *seen_cell = task->cells + hash;

                            if (seen_cell->used) {
                                space_see(seer_cell->first[seer_group],
                                          seen_cell->first[seen_group],
                                          see_func,
                                          radii,
                                          dims,
                                          thread_context);
                            }
                        }
                    }
                }
            }
            else
                assert(0); /* not implemented */
        }
        ++task->counter;
    }
}

static void _see(TayState *state, int pass_index) {
    static SeeTask tasks[TAY_MAX_THREADS];

    Space *space = &state->space;
    CpuGrid *grid = &space->cpu_grid;
    TayPass *pass = state->passes + pass_index;

    int4 kernel_radii;
    for (int i = 0; i < space->dims; ++i)
        kernel_radii.arr[i] = (int)ceilf(pass->radii.arr[i] / grid->cell_sizes.arr[i]);

    for (int i = 0; i < runner.count; ++i) {
        SeeTask *task = tasks + i;
        _init_see_task(task, pass, grid->cells, grid->first, i, space->dims, kernel_radii);
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

    /* sort agents into cells */
    {
        grid->first = 0;

        for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            TayAgentTag *next = space->first[group_i];
            while (next) {
                TayAgentTag *tag = next;
                next = next->next;

                ushort4 indices = _agent_position_to_cell_indices(TAY_AGENT_POSITION(tag),
                                                                  grid->grid_origin,
                                                                  grid->cell_sizes,
                                                                  space->dims);
                int hash = _cell_indices_to_hash(indices, space->dims);
                GridCell *cell = grid->cells + hash;
                cell->indices = indices;

                tag->next = cell->first[group_i];
                cell->first[group_i] = tag;

                if (cell->used == false) {
                    cell->next = grid->first;
                    grid->first = cell;
                    cell->used = true;
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
    for (GridCell *cell = grid->first; cell; cell = cell->next) {
        for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            space_return_agents(space, group_i, cell->first[group_i]);
            cell->first[group_i] = 0;
        }
        cell->used = false;
    }
}
