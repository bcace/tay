#include "state.h"
#include "thread.h"
#include "space_impl.h"
#include <assert.h>


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

    CpuSimple *s = &state->_space.cpu_simple;
    TayPass *p = state->passes + pass_index;
    int dims = state->_space.dims;

    for (int i = 0; i < runner.count; ++i) {
        TayAgentTag *b = s->threads[i].first[p->seen_group];

        for (int j = 0; j < runner.count; ++j) {
            TayAgentTag *a = s->threads[j].first[p->seer_group];

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

    CpuSimple *s = &state->_space.cpu_simple;
    TayPass *p = state->passes + pass_index;

    for (int i = 0; i < runner.count; ++i) {
        SimpleActTask *task = act_contexts + i;
        _init_simple_act_task(task, p, s->threads[i].first[p->act_group]);
        tay_thread_set_task(i, _act_func, task, p->context);
    }
    tay_runner_run();
}

void cpu_simple_step(TayState *state) {
    Space *space = &state->_space;
    int threads_count = runner.count;

    /* move all agents from space into thread storage */

    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        int rem = space->counts[group_i] % threads_count;
        int per_thread = (space->counts[group_i] - rem) / threads_count;

        space->cpu_simple.threads[0].first[group_i] = space->first[group_i];

        for (int thread_i = 1; thread_i < threads_count; ++thread_i) {
            int count = per_thread + (((thread_i - 1) < rem) ? 1 : 0);

            if (count == 0)
                space->cpu_simple.threads[thread_i].first[group_i] = 0;
            else {
                TayAgentTag *last = space->cpu_simple.threads[thread_i - 1].first[group_i];

                for (int i = 1; i < count; ++i) /* find the last tag of the previous thread */
                    last = last->next;

                space->cpu_simple.threads[thread_i].first[group_i] = last->next;
                last->next = 0;
            }
        }

        space->first[group_i] = 0;
        space->counts[group_i] = 0;
    }

    box_reset(&space->box, space->dims);

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

    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i)
        for (int thread_i = 0; thread_i < threads_count; ++thread_i)
            space_return_agents(space, group_i, space->cpu_simple.threads[thread_i].first[group_i]);
}
