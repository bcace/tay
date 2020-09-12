#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>


typedef struct {
    TayAgent *first[TAY_MAX_THREADS];
    int receiving_thread;
} SimpleGroup;

typedef struct {
    SimpleGroup groups[TAY_MAX_GROUPS];
} Simple;

static void _destroy(TaySpace *space) {
    free(space->storage);
}

static void _add(TaySpace *space, TayAgent *agent, int group) {
    Simple *s = space->storage;
    SimpleGroup *g = s->groups + group;
    int thread = (g->receiving_thread++) % runner.count;
    TayAgent *next = g->first[thread];
    agent->next = next;
    g->first[thread] = agent;
}

typedef struct {
    TayPass *pass;
    TayAgent *seer_agents;
    TayAgent *seen_agents;
    int dims;
} SimpleSeeTask;

static void _init_simple_see_task(SimpleSeeTask *see_context, TayPass *pass, TayAgent *seer_agents, TayAgent *seen_agents, int dims) {
    see_context->pass = pass;
    see_context->seer_agents = seer_agents;
    see_context->seen_agents = seen_agents;
    see_context->dims = dims;
}

static void _see_func(SimpleSeeTask *task, TayThreadContext *thread_context) {
    TAY_SEE_FUNC func = task->pass->see;
    float *radii = task->pass->radii;

    for (TayAgent *a = task->seer_agents; a; a = a->next) {
        float *pa = TAY_AGENT_POSITION(a);

        for (TayAgent *b = task->seen_agents; b; b = b->next) {
            float *pb = TAY_AGENT_POSITION(b);

            if (a == b) /* this can be removed for cases where beg_a != beg_b */
                continue;

            for (int k = 0; k < task->dims; ++k) {
                float d = pa[k] - pb[k];
                if (d < -radii[k] || d > radii[k])
                    goto OUTSIDE_RADII;
            }

            func(a, b, thread_context->context);

            OUTSIDE_RADII:;
        }
    }
}

static void _see(TaySpace *space, TayPass *pass, void *context) {
    static SimpleSeeTask tasks[TAY_MAX_THREADS];

    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i) {
        TayAgent *b = s->groups[pass->seen_group].first[i];

        for (int j = 0; j < runner.count; ++j) {
            TayAgent *a = s->groups[pass->seer_group].first[j];

            SimpleSeeTask *task = tasks + j;
            _init_simple_see_task(task, pass, a, b, space->dims);
            tay_thread_set_task(j, _see_func, task, context);
        }
        tay_runner_run();
    }
}

typedef struct {
    TayPass *pass;
    TayAgent *agents;
} SimpleActTask;

static void _init_simple_act_task(SimpleActTask *task, TayPass *pass, TayAgent *agents) {
    task->pass = pass;
    task->agents = agents;
}

static void _act_func(SimpleActTask *task, TayThreadContext *thread_context) {
    for (TayAgent *a = task->agents; a; a = a->next)
        task->pass->act(a, thread_context->context);
}

static void _act(TaySpace *space, TayPass *pass, void *context) {
    static SimpleActTask act_contexts[TAY_MAX_THREADS];

    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i) {
        SimpleActTask *task = act_contexts + i;
        _init_simple_act_task(task, pass, s->groups[pass->act_group].first[i]);
        tay_thread_set_task(i, _act_func, task, context);
    }
    tay_runner_run();
}

static void _post(TaySpace *space, void (*func)(void *), void *context) {
    Simple *s = space->storage;
    func(context);
}

static void _iter(TaySpace *space, int group, void (*func)(void *, void *), void *context) {
    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i)
        for (TayAgent *a = s->groups[group].first[i]; a; a = a->next)
            func(a, context);
}

static void _update(TaySpace *space) {
}

void space_simple_init(TaySpace *space, int dims) {
    Simple *s = calloc(1, sizeof(Simple));
    space_init(space, s, dims, _destroy, _add, _see, _act, _post, _iter, _update);
}
