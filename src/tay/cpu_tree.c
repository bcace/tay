#include "space.h"
#include "thread.h"
#include <assert.h>


typedef struct {
    CpuTree *tree;
    TayPass *pass;
    int counter; /* tree cell skipping counter so each thread processes different seer nodes */
} SeeTask;

static void _init_tree_see_task(SeeTask *task, CpuTree *tree, TayPass *pass, int thread_index) {
    task->tree = tree;
    task->pass = pass;
    task->counter = thread_index;
}

static void _thread_traverse_seen(CpuTree *tree, TreeCell *seer_cell, TreeCell *seen_cell, TayPass *pass, Box seer_box, TayThreadContext *thread_context) {
    for (int i = 0; i < tree->base.dims; ++i)
        if (seer_box.min.arr[i] > seen_cell->box.max.arr[i] || seer_box.max.arr[i] < seen_cell->box.min.arr[i])
            return;
    if (seen_cell->first[pass->seen_group]) /* if there are any "seen" agents */
        space_see(seer_cell->first[pass->seer_group], seen_cell->first[pass->seen_group], pass->see, pass->radii, tree->base.dims, thread_context);
    if (seen_cell->lo)
        _thread_traverse_seen(tree, seer_cell, seen_cell->lo, pass, seer_box, thread_context);
    if (seen_cell->hi)
        _thread_traverse_seen(tree, seer_cell, seen_cell->hi, pass, seer_box, thread_context);
}

static void _thread_traverse_seers(SeeTask *task, TreeCell *cell, TayThreadContext *thread_context) {
    if (task->counter % runner.count == 0) {
        if (cell->first[task->pass->seer_group]) { /* if there are any "seeing" agents */
            Box seer_box = cell->box;
            for (int i = 0; i < task->tree->base.dims; ++i) {
                seer_box.min.arr[i] -= task->pass->radii.arr[i];
                seer_box.max.arr[i] += task->pass->radii.arr[i];
            }
            _thread_traverse_seen(task->tree, cell, task->tree->base.cells, task->pass, seer_box, thread_context);
        }
    }
    ++task->counter;
    if (cell->lo)
        _thread_traverse_seers(task, cell->lo, thread_context);
    if (cell->hi)
        _thread_traverse_seers(task, cell->hi, thread_context);
}

static void _thread_see_traverse(SeeTask *task, TayThreadContext *thread_context) {
    _thread_traverse_seers(task, task->tree->base.cells, thread_context);
}

static void _see(TayState *state, int pass_index) {
    CpuTree *tree = &state->space.cpu_tree;
    TayPass *pass = state->passes + pass_index;

    static SeeTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        SeeTask *task = tasks + i;
        _init_tree_see_task(task, tree, pass, i);
        tay_thread_set_task(i, _thread_see_traverse, task, pass->context);
    }

    tay_runner_run();
}

typedef struct {
    CpuTree *tree;
    TayPass *pass;
    int counter; /* tree cell skipping counter so each thread processes different seer nodes */
} ActTask;

static void _init_tree_act_task(ActTask *task, CpuTree *tree, TayPass *pass, int thread_index) {
    task->tree = tree;
    task->pass = pass;
    task->counter = thread_index;
}

static void _thread_traverse_actors(ActTask *task, TreeCell *cell, TayThreadContext *thread_context) {
    if (task->counter % runner.count == 0) {
        if (cell->first[task->pass->seer_group]) { /* if there are any "seeing" agents */
            for (TayAgentTag *tag = cell->first[task->pass->seer_group]; tag; tag = tag->next)
                task->pass->act(tag, thread_context->context);
        }
    }
    ++task->counter;
    if (cell->lo)
        _thread_traverse_actors(task, cell->lo, thread_context);
    if (cell->hi)
        _thread_traverse_actors(task, cell->hi, thread_context);
}

static void _thread_act_traverse(ActTask *task, TayThreadContext *thread_context) {
    _thread_traverse_actors(task, task->tree->base.cells, thread_context);
}

static void _act(TayState *state, int pass_index) {
    CpuTree *tree = &state->space.cpu_tree;
    TayPass *pass = state->passes + pass_index;

    static ActTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        ActTask *task = tasks + i;
        _init_tree_act_task(task, tree, pass, i);
        tay_thread_set_task(i, _thread_act_traverse, task, pass->context);
    }

    tay_runner_run();
}

void cpu_tree_step(TayState *state) {

    /* build the tree */
    tree_update(&state->space, &state->space.cpu_tree.base);

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

    /* return agents to space and update the space box */
    tree_return_agents(&state->space, &state->space.cpu_tree.base);
}
