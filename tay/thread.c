#include "thread.h"
#include "state.h"
#include "tay.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>


TayRunner runner;

void tay_runner_init() {
    runner.count = 0;
    runner.state = TAY_RUNNER_IDLE;
}

static void _thread_work(TayThread *thread) {
    thread->task_func(thread->task, &thread->context);
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

static void _init_thread(TayThread *thread) {
    thread->run = 1;
}

void tay_thread_set_task(int index, void (*task_func)(void *, TayThreadContext *), void *task, void *context) {
    assert(index >= 0 && index < runner.count);
    TayThread *t = runner.threads + index;
    _init_thread(t);
    t->task_func = task_func;
    t->task = task;
    t->context.context = context;
}

void tay_runner_reset_stats() {
    for (int i = 0; i < TAY_MAX_THREADS; ++i) {
        TayThreadContext *c = &runner.threads[i].context;
        c->broad_see_phase = 0;
        c->narrow_see_phase = 0;
    }
}

void tay_runner_report_stats(int steps) {
    unsigned long long broad_see_phase = 0;
    unsigned long long narrow_see_phase = 0;
    for (int i = 0; i < runner.count; ++i) {
        TayThreadContext *c = &runner.threads[i].context;
        printf("  thread %d\n", i);
        printf("    see phases: %llu/%llu\n", c->narrow_see_phase, c->broad_see_phase);
        broad_see_phase += c->broad_see_phase;
        narrow_see_phase += c->narrow_see_phase;
    }
    printf("sum of see phases: %llu/%llu, %f per step\n", narrow_see_phase, broad_see_phase, narrow_see_phase / (double)steps);
}
