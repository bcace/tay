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
    // ushort4 indices;
} GridCell;

static inline ushort4 _agent_position_to_cell_indices(float4 pos, float4 orig, float4 size, int dims) {
    ushort4 indices;
    for (int i = 0; i < dims; ++i)
        indices.arr[i] = (unsigned short)floorf((pos.arr[i] - orig.arr[i]) / size.arr[i]);
    return indices;
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
    float4 grid_origin;
    float4 cell_sizes;
} _SeeTask;

static void _init_see_task(_SeeTask *task, TayPass *pass, GridCell *cells, GridCell *first_cell, int thread_index, int dims, int4 kernel_radii, float4 grid_origin, float4 cell_sizes) {
    task->pass = pass;
    task->cells = cells;
    task->first_cell = first_cell;
    task->counter = thread_index;
    task->dims = dims;
    task->kernel_radii = kernel_radii;
    task->grid_origin = grid_origin;
    task->cell_sizes = cell_sizes;
}

static void _see_func(_SeeTask *task, TayThreadContext *thread_context) {
    int seer_group = task->pass->seer_group;
    int seen_group = task->pass->seen_group;
    TAY_SEE_FUNC see_func = task->pass->see;
    float4 radii = task->pass->radii;
    int dims = task->dims;
    int4 kernel_radii = task->kernel_radii;
    float4 grid_origin = task->grid_origin;
    float4 cell_sizes = task->cell_sizes;

    int4 kernel_sizes;
    for (int i = 0; i < dims; ++i)
        kernel_sizes.arr[i] = kernel_radii.arr[i] * 2 + 1;

    for (GridCell *seer_cell = task->first_cell; seer_cell; seer_cell = seer_cell->next) {
        if (task->counter % runner.count == 0) {

            for (TayAgentTag *seer_agent = seer_cell->first[seer_group]; seer_agent; seer_agent = seer_agent->next) {
                float4 seer_p = TAY_AGENT_POSITION(seer_agent);

                ushort4 seer_indices = _agent_position_to_cell_indices(seer_p, grid_origin, cell_sizes, dims);
                ushort4 origin;
                for (int i = 0; i < dims; ++i)
                    origin.arr[i] = seer_indices.arr[i] - kernel_radii.arr[i];

                ushort4 seen_indices;
                if (dims == 3) {

                    for (int x = 0; x < kernel_sizes.x; ++x) {
                        seen_indices.x = origin.x + x;
                        for (int y = 0; y < kernel_sizes.y; ++y) {
                            seen_indices.y = origin.y + y;
                            for (int z = 0; z < kernel_sizes.z; ++z) {
                                seen_indices.z = origin.z + z;

                                int hash = _cell_indices_to_hash(seen_indices, dims);
                                GridCell *seen_cell = task->cells + hash;

                                if (seen_cell->used) {
                                    for (TayAgentTag *seen_agent = seen_cell->first[seen_group]; seen_agent; seen_agent = seen_agent->next) {
                                        float4 seen_p = TAY_AGENT_POSITION(seen_agent);

                                        if (seer_agent == seen_agent) /* this can be removed for cases where beg_a != beg_b */
                                            continue;

                                        for (int i = 0; i < dims; ++i) {
                                            float d = seer_p.arr[i] - seen_p.arr[i];
                                            if (d < -radii.arr[i] || d > radii.arr[i])
                                                goto SKIP_SEE;
                                        }

                                        see_func(seer_agent, seen_agent, thread_context->context);

                                        SKIP_SEE:;
                                    }
                                }
                            }
                        }
                    }
                }
                else
                    assert(0); /* not implemented */
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

    int4 kernel_radii;
    for (int i = 0; i < space->dims; ++i)
        kernel_radii.arr[i] = (int)ceilf(pass->radii.arr[i] / grid->cell_sizes.arr[i]);

    for (int i = 0; i < runner.count; ++i) {
        _SeeTask *task = tasks + i;
        _init_see_task(task, pass, grid->cells, grid->first, i, space->dims, kernel_radii, grid->grid_origin, grid->cell_sizes);
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
                int hash = _cell_indices_to_hash(indices, space->dims);
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
