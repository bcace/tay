#include "tree.h"
#include "thread.h"
#include <stdlib.h>


static TreeBase *_init(int dims, float *radii, int max_depth_correction) {
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

static void _thread_traverse_seen(TreeBase *space, Cell *seer_node, Cell *seen_node, TayPass *pass, Box seer_box, TayThreadContext *thread_context) {
    for (int i = 0; i < space->dims; ++i)
        if (seer_box.min[i] > seen_node->box.max[i] || seer_box.max[i] < seen_node->box.min[i])
            return;
    if (seen_node->first[pass->seen_group]) /* if there are any "seen" agents */
        tay_see(seer_node->first[pass->seer_group], seen_node->first[pass->seen_group], pass->see, pass->radii, space->dims, thread_context);
    if (seen_node->lo)
        _thread_traverse_seen(space, seer_node, seen_node->lo, pass, seer_box, thread_context);
    if (seen_node->hi)
        _thread_traverse_seen(space, seer_node, seen_node->hi, pass, seer_box, thread_context);
}

static void _thread_traverse_seers(SeeTask *task, Cell *cell, TayThreadContext *thread_context) {
    if (task->counter % runner.count == 0) {
        if (cell->first[task->pass->seer_group]) { /* if there are any "seeing" agents */
            Box seer_box = cell->box;
            for (int i = 0; i < task->space->dims; ++i) {
                seer_box.min[i] -= task->pass->radii[i];
                seer_box.max[i] += task->pass->radii[i];
            }
            _thread_traverse_seen(task->space, cell, task->space->cells, task->pass, seer_box, thread_context);
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

static void _see(TayState *state, int pass_index) { // TaySpaceContainer *container, TayPass *pass) {
    TreeBase *t = state->space.storage; // container->storage;
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

void space_cpu_tree_init(TaySpaceContainer *container, int dims, float *radii, int max_depth_correction) {
    space_container_init(container, _init(dims, radii, max_depth_correction), dims, _destroy);
    container->add = _add;
    container->see = _see,
    container->act = _act;
    container->on_step_start = _on_step_start;
}
