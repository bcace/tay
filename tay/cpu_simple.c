#include "space.h"
#include "thread.h"
#include <string.h>


typedef struct {
    TayPass *pass;
    int thread_i;
} SimpleSeeTask;

static void _init_simple_see_task(SimpleSeeTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->thread_i = thread_i;
}

void cpu_simple_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, TayThreadContext *thread_context) {
    
    AgentsSlice seen_slice = {
        pass->seen_group->storage,
        pass->seen_group->agent_size,
        0,
        pass->seen_group->space.count,
    };

    pass->new_pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
}

static void _see_func(SimpleSeeTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    TayGroup *seer_group = pass->seer_group;
    TayGroup *seen_group = pass->seen_group;
    CpuSimple *seer_simple = &seer_group->space.cpu_simple;

    int min_dims = (seer_group->space.dims < seen_group->space.dims) ?
                   seer_group->space.dims :
                   seen_group->space.dims;

    int agents_per_thread = pass->seer_group->space.count / runner.count;
    unsigned beg_seer_i = agents_per_thread * task->thread_i;
    unsigned end_seer_i = (task->thread_i < runner.count - 1) ?
                          beg_seer_i + agents_per_thread :
                          pass->seer_group->space.count;

    AgentsSlice seer_slice = {
        seer_group->storage,
        seer_group->agent_size,
        beg_seer_i,
        end_seer_i,
    };

    pass->new_seen_func(task->pass, seer_slice, (Box){0}, min_dims, thread_context);
}

void cpu_simple_see(TayPass *pass) {

    if (pass->seer_group == pass->seen_group) {
        TayGroup *seen_group = pass->seen_group;
        memcpy(seen_group->sort_storage, seen_group->storage, seen_group->agent_size * seen_group->space.count);
    }

    static SimpleSeeTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        SimpleSeeTask *task = tasks + i;
        _init_simple_see_task(task, pass, i);
        tay_thread_set_task(i, _see_func, task, pass->context);
    }

    tay_runner_run();
}

typedef struct {
    TayPass *pass;
    int thread_i;
} ActTask;

static void _init_act_task(ActTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->thread_i = thread_i;
}

static void _act_func(ActTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;

    int agents_per_thread = pass->act_group->space.count / runner.count;
    unsigned beg_agent_i = agents_per_thread * task->thread_i;
    unsigned end_agent_i = (task->thread_i < runner.count - 1) ?
                           beg_agent_i + agents_per_thread :
                           pass->act_group->space.count;

    for (unsigned agent_i = beg_agent_i; agent_i < end_agent_i; ++agent_i) {
        void *agent = (char *)pass->act_group->storage + pass->act_group->agent_size * agent_i;
        pass->act(agent, thread_context->context);
    }
}

void cpu_simple_act(TayPass *pass) {
    static ActTask act_contexts[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        ActTask *task = act_contexts + i;
        _init_act_task(task, pass, i);
        tay_thread_set_task(i, _act_func, task, pass->context);
    }

    tay_runner_run();
}

void cpu_simple_sort(TayGroup *group) {
}

void cpu_simple_unsort(TayGroup *group) {
}
