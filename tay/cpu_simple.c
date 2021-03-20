#include "space.h"
#include "thread.h"
#include <assert.h>


typedef struct {
    TayPass *pass;
    TayAgentTag *seer_agents;
    TayAgentTag *seen_agents;
    SEE_PAIRING_FUNC pairing_func;
    int dims;
} SimpleSeeTask;

static void _init_simple_see_task(SimpleSeeTask *task, TayPass *pass, TayAgentTag *seer_agents, TayAgentTag *seen_agents, SEE_PAIRING_FUNC pairing_func, int dims) {
    task->pass = pass;
    task->seer_agents = seer_agents;
    task->seen_agents = seen_agents;
    task->pairing_func = pairing_func;
    task->dims = dims;
}

static void _see_func(SimpleSeeTask *task, TayThreadContext *thread_context) {
    task->pairing_func(task->seer_agents, task->seen_agents, task->pass->see, task->pass->radii, task->dims, thread_context);
}

void cpu_simple_see(TayPass *pass) {
    static SimpleSeeTask tasks[TAY_MAX_THREADS];

    if (pass->seer_space == pass->seen_space) {
        Space *space = pass->seer_space;

        for (int i = 0; i < runner.count; ++i) {
            TayAgentTag *b = space->cpu_simple.first[i];

            for (int j = 0; j < runner.count; ++j) {
                TayAgentTag *a = space->cpu_simple.first[j];

                SimpleSeeTask *task = tasks + j;
                _init_simple_see_task(task, pass, a, b, pass->pairing_func, space->dims);
                tay_thread_set_task(j, _see_func, task, pass->context);
            }

            tay_runner_run();
        }
    }
}

typedef struct {
    TayPass *pass;
    TayAgentTag *agents;
} ActTask;

static void _init_act_task(ActTask *task, TayPass *pass, TayAgentTag *agents) {
    task->pass = pass;
    task->agents = agents;
}

static void _act_func(ActTask *task, TayThreadContext *thread_context) {
    for (TayAgentTag *tag = task->agents; tag; tag = tag->next)
        task->pass->act(tag, thread_context->context);
}

void cpu_simple_act(TayPass *pass) {
    static ActTask act_contexts[TAY_MAX_THREADS];

    CpuSimple *simple = &pass->act_space->cpu_simple;

    for (int i = 0; i < runner.count; ++i) {
        ActTask *task = act_contexts + i;
        _init_act_task(task, pass, simple->first[i]);
        tay_thread_set_task(i, _act_func, task, pass->context);
    }
    tay_runner_run();
}

void cpu_simple_sort(TayGroup *group) {
    Space *space = &group->new_space;
    space->cpu_simple.first = space->shared;

    int rem = space->count % runner.count;
    int per_thread = (space->count - rem) / runner.count;

    space->cpu_simple.first[0] = space->first;

    for (int thread_i = 1; thread_i < runner.count; ++thread_i) {
        int count = per_thread + (((thread_i - 1) < rem) ? 1 : 0);

        if (count == 0)
            space->cpu_simple.first[thread_i] = 0;
        else {
            TayAgentTag *last = space->cpu_simple.first[thread_i - 1];

            for (int i = 1; i < count; ++i) /* find the last tag of the previous thread */
                last = last->next;

            space->cpu_simple.first[thread_i] = last->next;
            last->next = 0;
        }
    }

    space->first = 0;
    space->count = 0;
}

void cpu_simple_unsort(TayGroup *group) {
    Space *space = &group->new_space;
    box_reset(&space->box, space->dims);

    for (int thread_i = 0; thread_i < runner.count; ++thread_i)
        space_return_agents(space, space->cpu_simple.first[thread_i], group->is_point);
}
