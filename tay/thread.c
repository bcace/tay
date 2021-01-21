#include "thread.h"
#include "state.h"
#include "tay.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>


TayRunner runner;

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

static void _reset_telemetry() {
    TayTelemetry *t = &runner.telemetry;
    t->b_see_sum = 0;
    t->n_see_sum = 0;
    t->rel_dev_mean_sum = 0.0;
    t->rel_dev_max_sum = 0.0;
    t->rel_dev_max = 0.0;
    t->steps_count = 0;
    for (int i = 0; i < TAY_MAX_THREADS; ++i) {
        TayThreadContext *ctx = &runner.threads[i].context;
        ctx->broad_see_phase = 0;
        ctx->narrow_see_phase = 0;
    }
}

void _start_threads(int threads_count) {
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
    _reset_telemetry();
}

void tay_threads_start() {
    _start_threads(GetMaximumProcessorCount(ALL_PROCESSOR_GROUPS));
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

void _stop_threads() {
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

void tay_threads_stop() {
    _stop_threads();
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

/* Updates telemetry for one simulation step. */
void tay_threads_update_telemetry() {
    TayTelemetry *t = &runner.telemetry;

    double sum = 0.0;
    for (int i = 0; i < runner.count; ++i) {
        TayThreadContext *c = &runner.threads[i].context;
        sum += (double)c->broad_see_phase;
        t->b_see_sum += c->broad_see_phase;
        t->n_see_sum += c->narrow_see_phase;
    }
    double mean = sum / (double)runner.count;

    double dev_sum = 0.0;
    double dev_max = 0.0;
    for (int i = 0; i < runner.count; ++i) {
        TayThreadContext *c = &runner.threads[i].context;
        double dev = fabs(c->broad_see_phase - mean);
        dev_sum += dev;
        if (dev > dev_max)
            dev_max = dev;
    }
    double dev_mean = dev_sum / (double)runner.count;
    double rel_dev_mean = dev_mean / mean;
    double rel_dev_max = dev_max / mean;

    t->rel_dev_mean_sum += rel_dev_mean;
    t->rel_dev_max_sum += rel_dev_max;
    if (rel_dev_max > t->rel_dev_max)
        t->rel_dev_max = rel_dev_max;

    ++t->steps_count;

    for (int i = 0; i < runner.count; ++i) {
        TayThreadContext *c = &runner.threads[i].context;
        c->broad_see_phase = 0;
        c->narrow_see_phase = 0;
    }
}

static double _max(double a, double b) {
    return (a > b) ? a : b;
}

// TODO: make the entire function into a macro that optionally tuns to a no-op
void tay_threads_report_telemetry(unsigned steps_between_reports) {
#if TAY_TELEMETRY
    TayTelemetry *t = &runner.telemetry;
    if ((steps_between_reports != 0) && (t->steps_count % steps_between_reports))
        return;

    printf("    telemetry:\n");
    printf("      thread see phase balancing (potential see interactions per thread per step):\n");
    printf("        mean relative deviation (averaged over steps): %.2f%%\n", t->rel_dev_mean_sum * 100.0 / (double)t->steps_count);
    printf("        max relative deviation (averaged over steps): %.2f%%\n", t->rel_dev_max_sum * 100.0 / (double)t->steps_count);
    printf("        max relative deviation: %.2f%%\n", t->rel_dev_max * 100.0);
    printf("      see interaction culling efficiency (actual / potential): %.2f%%\n", t->n_see_sum * 100.0 / (double)t->b_see_sum);
    printf("      mean actual see interactions per step: %.2f\n", t->n_see_sum / (double)t->steps_count);

    _reset_telemetry();
#endif
}
