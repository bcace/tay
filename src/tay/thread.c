#include "thread.h"
#include "state.h"
#include "tay.h"
#include <stdlib.h>
#include <assert.h>
#include <windows.h>


TayRunner runner;

void tay_runner_init() {
    runner.count = 0;
    runner.state = TAY_RUNNER_IDLE;
}

void tay_runner_clear_stats() {
    for (int i = 0; i < TAY_MAX_THREADS; ++i) {
        runner.threads[i].context.all_see_runs = 0;
        runner.threads[i].context.useful_see_runs = 0;
    }
}

static void _thread_work(TayThread *thread) {
    if (thread->pass == 0)
        return;
    else if (thread->pass->type == TAY_PASS_SEE) {

        TAY_SEE_FUNC see = thread->pass->see;
        float *radii = thread->pass->radii;

        int tasks_count = 1;
        TayThreadSeeTask *tasks = &thread->see_task;

        if (thread->see_tasks_count != -1) {
            tasks_count = thread->see_tasks_count;
            tasks = thread->see_tasks;
        }

        for (int task_i = 0; task_i < tasks_count; ++task_i) {
            TayThreadSeeTask *task = tasks + task_i;

            for (TayAgent *a = task->seer_agents; a; a = a->next) {
                float *pa = TAY_AGENT_POSITION(a);

                int seen_bundles_count = 1;
                TayAgent **seen_bundles = &task->seen_agents;

                if (task->seen_bundles_count != -1) {
                    seen_bundles_count = task->seen_bundles_count;
                    seen_bundles = task->seen_bundles;
                }

                for (int bundle_i = 0; bundle_i < seen_bundles_count; ++bundle_i) {

                    for (TayAgent *b = seen_bundles[bundle_i]; b; b = b->next) {
                        float *pb = TAY_AGENT_POSITION(b);

                        if (a == b) /* this can be removed for cases where beg_a != beg_b */
                            continue;

                        for (int k = 0; k < thread->dims; ++k) {
                            float d = pa[k] - pb[k];
                            if (d < -radii[k] || d > radii[k])
                                goto OUTSIDE_RADII;
                        }

                        see(a, b, &thread->context);

                        OUTSIDE_RADII:;
                    }
                }
            }
        }
    }
    else if (thread->pass->type == TAY_PASS_ACT) {
        for (TayAgent *a = thread->act_agents; a; a = a->next)
            thread->pass->act(a, &thread->context);
    }
}

unsigned int WINAPI _thread_func(TayThread *thread) {
    while (1) {
        WaitForSingleObject(thread->beg_semaphore, INFINITE);
        if (thread->run == 0) {
            ReleaseSemaphore(thread->end_semaphore, 1, 0);
            return 0;
        }
        _thread_work(thread);
        ReleaseSemaphore(thread->end_semaphore, 1, 0);
    }
    return 0;
}

void tay_runner_start_threads(int threads_count) {
    assert(runner.state == TAY_RUNNER_IDLE);
    for (int i = 0; i < threads_count; ++i) {
        TayThread *t = runner.threads + i;
        t->run = 1;
        t->beg_semaphore = CreateSemaphore(0, 0, 1, 0);
        t->end_semaphore = CreateSemaphore(0, 0, 1, 0);
        t->thread = CreateThread(0, 0, _thread_func, t, 0, 0);
    }
    runner.count = threads_count;
    runner.state = TAY_RUNNER_WAITING;
}

void tay_runner_run() {
    /* TODO: ensure that user initialized all tasks before running, currently we just assume
    that the number of tasks is equal to the number of threads, and we don't check that anywhere. */
    assert(runner.state == TAY_RUNNER_WAITING);
    Handle end_semaphores[TAY_MAX_THREADS];
    for (int i = 0; i < runner.count; ++i) {
        TayThread *t = runner.threads + i;
        ReleaseSemaphore(t->beg_semaphore, 1, 0);
        end_semaphores[i] = t->end_semaphore;
    }
    WaitForMultipleObjects(runner.count, end_semaphores, 1, INFINITE);
}

void tay_runner_run_no_threads() {
    assert(runner.state == TAY_RUNNER_WAITING);
    for (int i = 0; i < runner.count; ++i)
        _thread_work(runner.threads + i);
}

void tay_runner_stop_threads() {
    assert(runner.state == TAY_RUNNER_WAITING);
    for (int i = 0; i < runner.count; ++i) {
        TayThread *t = runner.threads + i;
        t->run = 0;
    }
    tay_runner_run(runner); /* wait until all threads signal that they're done */
    for (int i = 0; i < runner.count; ++i) {
        TayThread *t = runner.threads + i;
        CloseHandle(t->thread);
        CloseHandle(t->beg_semaphore);
        CloseHandle(t->end_semaphore);
    }
    runner.state = TAY_RUNNER_IDLE;
}

static void _init_thread(TayThread *thread, void *context) {
    thread->run = 1;
    thread->context.context = context;
}

void tay_thread_set_see(int index, void *context,
                        TayAgent *seer_agents,
                        TayAgent *seen_agents,
                        TayPass *pass,
                        int dims) {
    assert(index >= 0 && index < runner.count);
    TayThread *t = runner.threads + index;
    _init_thread(t, context);
    t->see_task.seer_agents = seer_agents;
    t->see_task.seen_agents = seen_agents;
    t->see_task.seen_bundles_count = -1;
    t->see_tasks_count = -1;
    t->pass = pass;
    t->dims = dims;
}

void tay_thread_set_see_multiple(int index, void *context,
                                 TayThreadSeeTask *see_tasks,
                                 int see_tasks_count,
                                 TayPass *pass,
                                 int dims) {
    assert(index >= 0 && index < runner.count);
    TayThread *t = runner.threads + index;
    _init_thread(t, context);
    t->see_tasks = see_tasks;
    t->see_tasks_count = see_tasks_count;
    t->pass = pass;
    t->dims = dims;
}

void tay_thread_set_act(int index, void *context,
                        TayAgent *act_agents,
                        TayPass *pass) {
    assert(index >= 0 && index < runner.count);
    TayThread *t = runner.threads + index;
    _init_thread(t, context);
    t->act_agents = act_agents;
    t->pass = pass;
}
