#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>


typedef struct {
    TayAgentTag *first[TAY_MAX_THREADS];
    int receiving_thread;
} SimpleGroup;

typedef struct {
    SimpleGroup groups[TAY_MAX_GROUPS];
} Simple;

static Simple *_init() {
    return calloc(1, sizeof(Simple));
}

static void _destroy(TaySpace *space) {
    free(space->storage);
}

static void _add(TaySpace *space, TayAgentTag *agent, int group, int index) {
    Simple *s = space->storage;
    SimpleGroup *g = s->groups + group;
    int thread = (g->receiving_thread++) % runner.count;
    TayAgentTag *next = g->first[thread];
    agent->next = next;
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

static void _see(TaySpace *space, TayPass *pass) {
    static SimpleSeeTask tasks[TAY_MAX_THREADS];

    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i) {
        TayAgentTag *b = s->groups[pass->seen_group].first[i];

        for (int j = 0; j < runner.count; ++j) {
            TayAgentTag *a = s->groups[pass->seer_group].first[j];

            SimpleSeeTask *task = tasks + j;
            _init_simple_see_task(task, pass, a, b, space->dims);
            tay_thread_set_task(j, _see_func, task, pass->context);
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
        task->pass->act(TAY_AGENT_DATA(a), thread_context->context);
}

static void _act(TaySpace *space, TayPass *pass) {
    static SimpleActTask act_contexts[TAY_MAX_THREADS];

    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i) {
        SimpleActTask *task = act_contexts + i;
        _init_simple_act_task(task, pass, s->groups[pass->act_group].first[i]);
        tay_thread_set_task(i, _act_func, task, pass->context);
    }
    tay_runner_run();
}

static void _update(TaySpace *space) {
}

void space_simple_init(TaySpace *space, int dims) {
    space_init(space, _init(), dims, _destroy, _add, _see, _act, _update);
}
