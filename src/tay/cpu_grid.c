#include "space.h"
#include "thread.h"
#include <math.h>
#include <assert.h>


typedef struct GridCell {
    struct GridCell *next;
    TayAgentTag *first[TAY_MAX_GROUPS];
    int used;
} GridCell;

static inline int _agent_position_to_cell_index(float p, float o, float s) {
    return (int)floorf((p - o) / s);
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

void cpu_grid_prepare(TayState *state) {
    Space *space = &state->space;
    CpuGrid *grid = &space->cpu_grid;
    grid->cells = space_get_cell_arena(space, TAY_MAX_CELLS * sizeof(GridCell), true);
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

                int hash = _agent_position_to_hash(grid, TAY_AGENT_POSITION(tag), space->dims);
                GridCell *cell = grid->cells + hash;

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
            /*_see(state, i)*/;
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
