#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>


typedef struct {
    TayAgentTag *first[TAY_MAX_THREADS];
    int receiving_thread;
} Group;

typedef struct {
    Group groups[TAY_MAX_GROUPS];
} Space;

static Space *_init() {
    return calloc(1, sizeof(Space));
}

static void _destroy(TaySpaceContainer *space) {
    free(space->storage);
}

static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
    Space *s = container->storage;
    Group *g = s->groups + group;
    int thread = (g->receiving_thread++) % runner.count;
    agent->next = g->first[thread];
    g->first[thread] = agent;
}

typedef struct {
    TayPass *pass;
    TayAgentTag *seer_agents;
    TayAgentTag *seen_agents;
    int dims;
} SimpleSeeTask;

static void _init_simple_see_task(SimpleSeeTask *task, TayPass *pass, TayAgentTag *seer_agents, TayAgentTag *seen_agents, int dims) {
    task->pass = pass;
    task->seer_agents = seer_agents;
    task->seen_agents = seen_agents;
    task->dims = dims;
}

static void _see_func(SimpleSeeTask *task, TayThreadContext *thread_context) {
    tay_see(task->seer_agents, task->seen_agents, task->pass->see, task->pass->radii, task->dims, thread_context);
}

static void _see(TayState *state, int pass_index) {
    static SimpleSeeTask tasks[TAY_MAX_THREADS];

    Space *s = state->space.storage;
    TayPass *p = state->passes + pass_index;
    int dims = state->space.dims;

    for (int i = 0; i < runner.count; ++i) {
        TayAgentTag *b = s->groups[p->seen_group].first[i];

        for (int j = 0; j < runner.count; ++j) {
            TayAgentTag *a = s->groups[p->seer_group].first[j];

            SimpleSeeTask *task = tasks + j;
            _init_simple_see_task(task, p, a, b, dims);
            tay_thread_set_task(j, _see_func, task, p->context);
        }
        tay_runner_run();
    }
}

typedef struct {
    TayPass *pass;
    TayAgentTag *agents;
} SimpleActTask;

static void _init_simple_act_task(SimpleActTask *task, TayPass *pass, TayAgentTag *agents) {
    task->pass = pass;
    task->agents = agents;
}

static void _act_func(SimpleActTask *task, TayThreadContext *thread_context) {
    for (TayAgentTag *a = task->agents; a; a = a->next)
        task->pass->act(a, thread_context->context);
}

static void _act(TayState *state, int pass_index) {
    static SimpleActTask act_contexts[TAY_MAX_THREADS];

    Space *s = state->space.storage;
    TayPass *p = state->passes + pass_index;

    for (int i = 0; i < runner.count; ++i) {
        SimpleActTask *task = act_contexts + i;
        _init_simple_act_task(task, p, s->groups[p->act_group].first[i]);
        tay_thread_set_task(i, _act_func, task, p->context);
    }
    tay_runner_run();
}

void space_simple_init(TaySpaceContainer *container, int dims) {
    space_container_init(container, _init(), dims, _destroy);
    container->add = _add;
    container->see = _see;
    container->act = _act;
}
