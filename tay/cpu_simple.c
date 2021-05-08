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

    pass->pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
}

static void _see_func(SimpleSeeTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    TayGroup *seer_group = pass->seer_group;
    TayGroup *seen_group = pass->seen_group;
    CpuSimple *seer_simple = &seer_group->space.cpu_simple;

    int min_dims = (seer_group->space.dims < seen_group->space.dims) ?
                   seer_group->space.dims :
                   seen_group->space.dims;

    TayRange seers_range = tay_threads_range(pass->seer_group->space.count, task->thread_i);

    AgentsSlice seer_slice = {
        seer_group->storage,
        seer_group->agent_size,
        seers_range.beg,
        seers_range.end,
    };

    pass->seen_func(task->pass, seer_slice, (Box){0}, min_dims, thread_context);
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

void cpu_simple_sort(TayGroup *group) {
}

void cpu_simple_unsort(TayGroup *group) {
}
