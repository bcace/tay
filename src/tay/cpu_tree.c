#include "tree.h"
#include "state.h"
#include "thread.h"
#include <stdlib.h>
#include <assert.h>


static TreeBase *_init(int dims, float4 radii, int max_depth_correction) {
    TreeBase *tree = malloc(sizeof(TreeBase));
    tree_init(tree, dims, radii, max_depth_correction);
    return tree;
}

static void _destroy(TaySpaceContainer *container) {
    tree_destroy(container->storage);
}

static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
    tree_add(container->storage, agent, group);
}

static void _on_step_start(TayState *state) {
    tree_update(state->space.storage);
}

typedef struct {
    TreeBase *space;
    TayPass *pass;
    int counter; /* tree cell skipping counter so each thread processes different seer nodes */
} SeeTask;

static void _init_tree_see_task(SeeTask *t, TreeBase *space, TayPass *pass, int thread_index) {
    t->space = space;
    t->pass = pass;
    t->counter = thread_index;
}

static void _thread_traverse_seen(TreeBase *tree, Cell *seer_cell, Cell *seen_cell, TayPass *pass, Box seer_box, TayThreadContext *thread_context) {
    for (int i = 0; i < tree->dims; ++i)
        if (seer_box.min.arr[i] > seen_cell->box.max.arr[i] || seer_box.max.arr[i] < seen_cell->box.min.arr[i])
            return;
    if (seen_cell->first[pass->seen_group]) /* if there are any "seen" agents */
        tay_see(seer_cell->first[pass->seer_group], seen_cell->first[pass->seen_group], pass->see, pass->radii, tree->dims, thread_context);
    if (seen_cell->lo)
        _thread_traverse_seen(tree, seer_cell, seen_cell->lo, pass, seer_box, thread_context);
    if (seen_cell->hi)
        _thread_traverse_seen(tree, seer_cell, seen_cell->hi, pass, seer_box, thread_context);
}

static void _thread_traverse_seen_non_recursive(TreeBase *tree, Cell *seer_cell, Cell *seen_cell, TayPass *pass, Box seer_box, TayThreadContext *thread_context) {
    Cell *stack[16];
    int stack_size = 0;
    stack[stack_size++] = tree->cells;

    float4 radii = pass->radii;
    int dims = tree->dims;

    while (stack_size) {
        Cell *seen_cell = stack[--stack_size];

        while (seen_cell) {

            for (int i = 0; i < tree->dims; ++i) {
                if (seer_box.min.arr[i] > seen_cell->box.max.arr[i] || seer_box.max.arr[i] < seen_cell->box.min.arr[i]) {
                    seen_cell = 0;
                    goto SKIP_CELL;
                }
            }

            if (seen_cell->hi) {
                stack[stack_size++] = seen_cell->hi;
                assert(stack_size < 16);
            }

            TayAgentTag *seer_agent = seer_cell->first[pass->seer_group];
            while (seer_agent) {
                float4 seer_p = *TAY_AGENT_POSITION(seer_agent);

                TayAgentTag *seen_agent = seen_cell->first[pass->seen_group];
                while (seen_agent) {
                    float4 seen_p = *TAY_AGENT_POSITION(seen_agent);

                    if (seer_agent == seen_agent)
                        goto SKIP_SEE;

                    for (int k = 0; k < dims; ++k) {
                        float d = seer_p.arr[k] - seen_p.arr[k];
                        if (d < -radii.arr[k] || d > radii.arr[k])
                            goto SKIP_SEE;
                    }

                    pass->see((void *)seer_agent, (void *)seen_agent, thread_context->context);

                    SKIP_SEE:;
                    seen_agent = seen_agent->next;
                }

                seer_agent = seer_agent->next;
            }

            seen_cell = seen_cell->lo;

            SKIP_CELL:;
        }
    }
}

static void _thread_traverse_seers(SeeTask *task, Cell *cell, TayThreadContext *thread_context) {
    if (task->counter % runner.count == 0) {
        if (cell->first[task->pass->seer_group]) { /* if there are any "seeing" agents */
            Box seer_box = cell->box;
            for (int i = 0; i < task->space->dims; ++i) {
                seer_box.min.arr[i] -= task->pass->radii.arr[i];
                seer_box.max.arr[i] += task->pass->radii.arr[i];
            }
            _thread_traverse_seen(task->space, cell, task->space->cells, task->pass, seer_box, thread_context);
            // _thread_traverse_seen_non_recursive(task->space, cell, task->space->cells, task->pass, seer_box, thread_context);
        }
    }
    ++task->counter;
    if (cell->lo)
        _thread_traverse_seers(task, cell->lo, thread_context);
    if (cell->hi)
        _thread_traverse_seers(task, cell->hi, thread_context);
}

static void _thread_see_traverse(SeeTask *task, TayThreadContext *thread_context) {
    _thread_traverse_seers(task, task->space->cells, thread_context);
}

static void _see(TayState *state, int pass_index) {
    TreeBase *t = state->space.storage;
    TayPass *p = state->passes + pass_index;

    static SeeTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        SeeTask *task = tasks + i;
        _init_tree_see_task(task, t, p, i);
        tay_thread_set_task(i, _thread_see_traverse, task, p->context);
    }

    tay_runner_run();
}

typedef struct {
    TayPass *pass;
    TayAgentTag *agents;
} ActTask;

static void _init_tree_act_task(ActTask *t, TayPass *pass, TayAgentTag *agents) {
    t->pass = pass;
    t->agents = agents;
}

static void _act_func(ActTask *t, TayThreadContext *thread_context) {
    for (TayAgentTag *a = t->agents; a; a = a->next)
        t->pass->act(a, thread_context->context);
}

static void _dummy_act_func(ActTask *t, TayThreadContext *thread_context) {}

static void _traverse_actors(Cell *cell, TayPass *pass) {
    ActTask task;
    _init_tree_act_task(&task, pass, cell->first[pass->act_group]);
    tay_thread_set_task(0, _act_func, &task, pass->context);
    for (int i = 1; i < runner.count; ++i)
        tay_thread_set_task(i, _dummy_act_func, 0, 0); // TODO: this is horrible, act here is still not parallelized!!!
    tay_runner_run_no_threads();
    if (cell->lo)
        _traverse_actors(cell->lo, pass);
    if (cell->hi)
        _traverse_actors(cell->hi, pass);
}

static void _act(TayState *state, int pass_index) {
    TreeBase *s = state->space.storage;
    TayPass *p = state->passes + pass_index;
    _traverse_actors(s->cells, p);
}

void space_cpu_tree_init(TaySpaceContainer *container, int dims, float4 radii, int max_depth_correction) {
    space_container_init(container, _init(dims, radii, max_depth_correction), dims, _destroy);
    container->add = _add;
    container->see = _see,
    container->act = _act;
    container->on_step_start = _on_step_start;
}
