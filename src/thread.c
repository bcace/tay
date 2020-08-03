#include "thread.h"
#include "state.h"
#include <stdlib.h>
#include <assert.h>
#include <windows.h>


TayRunner runner;

void tay_runner_init() {
    runner.count = 0;
    runner.state = TAY_RUNNER_IDLE;
}

unsigned int WINAPI _thread_func(TayThread *thread) {
    while (1) {
        WaitForSingleObject(thread->beg_semaphore, INFINITE);
        if (thread->run == 0) {
            ReleaseSemaphore(thread->end_semaphore, 1, 0);
            return 0;
        }
        if (thread->perception) {
            for (TayAgent *a = thread->beg_a; a != thread->end_a; a = a->next)
                for (TayAgent *b = thread->beg_b; b != thread->end_b; b = b->next)
                    if (a != b) /* this can be removed for cases where beg_a != beg_b */
                        thread->perception(a, b, thread->context);
        }
        else if (thread->action) {
            for (TayAgent *a = thread->beg_a; a != thread->end_a; a = a->next)
                thread->action(a, thread->context);
        }
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
    assert(runner.state == TAY_RUNNER_WAITING);
    Handle end_semaphores[TAY_MAX_THREADS];
    for (int i = 0; i < runner.count; ++i) {
        TayThread *t = runner.threads + i;
        ReleaseSemaphore(t->beg_semaphore, 1, 0);
        end_semaphores[i] = t->end_semaphore;
    }
    WaitForMultipleObjects(runner.count, end_semaphores, 1, INFINITE);
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
    thread->context = context;
}

void tay_thread_set_perception(int index, void *context,
                               void (*func)(TayAgent *a, TayAgent *b, void *context),
                               TayAgent *beg_a, TayAgent *end_a,
                               TayAgent *beg_b, TayAgent *end_b) {
    assert(index >= 0 && index < runner.count);
    TayThread *t = runner.threads + index;
    _init_task(t, context);
    t->beg_a = beg_a;
    t->end_a = end_a;
    t->beg_b = beg_b;
    t->end_b = end_b;
    t->action = 0;
    t->perception = func;
}

void tay_thread_set_action(int index, void *context,
                           void (*func)(TayAgent *a, void *context),
                           TayAgent *beg_a, TayAgent *end_a) {
    assert(index >= 0 && index < runner.count);
    TayThread *t = runner.threads + index;
    _init_task(t, context);
    t->beg_a = beg_a;
    t->end_a = end_a;
    t->action = func;
    t->perception = 0;
}
