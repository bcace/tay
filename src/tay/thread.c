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
    if (thread->pass->type == TAY_PASS_SEE) {

        TAY_SEE_FUNC see = thread->pass->see;
        float *radii = thread->pass->radii;

        for (TayAgent *a = thread->beg_a; a; a = a->next) { /* seer agents */
            float *pa = TAY_AGENT_POSITION(a);

            for (TayAgent *b = thread->beg_b; b; b = b->next) { /* seen agents */
                float *pb = TAY_AGENT_POSITION(b);

                if (a == b) /* this can be removed for cases where beg_a != beg_b */
                    continue;

                for (int i = 0; i < thread->dims; ++i) {
                    float d = pa[i] - pb[i];
                    if (d < -radii[i] || d > radii[i])
                        goto OUTSIDE_RADII;
                }

                see(a, b, &thread->context);

                OUTSIDE_RADII:;
            }
        }
    }
    else if (thread->pass->type == TAY_PASS_ACT) {
        for (TayAgent *a = thread->beg_a; a; a = a->next)
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

static void _init_task(TayThread *thread, void *context) {
    thread->run = 1;
    thread->context.context = context;
}

void tay_thread_set_see(int index, void *context,
                               TayAgent *beg_a, TayAgent *beg_b,
                               TayPass *pass,
                               int dims) {
    assert(index >= 0 && index < runner.count);
    TayThread *t = runner.threads + index;
    _init_task(t, context);
    t->beg_a = beg_a;
    t->beg_b = beg_b;
    t->pass = pass;
    t->dims = dims;
}

void tay_thread_set_act(int index, void *context,
                           TayAgent *beg_a,
                           TayPass *pass) {
    assert(index >= 0 && index < runner.count);
    TayThread *t = runner.threads + index;
    _init_task(t, context);
    t->beg_a = beg_a;
    t->pass = pass;
}
