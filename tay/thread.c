#include "thread.h"
#include "state.h"
#include "tay.h"
#include <math.h>
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

static void _reset_telemetry() {
    TayTelemetry *t = &runner.telemetry;
    t->thread_runs_count = 0;
    t->samples_count = 0;
    t->steps_count = 0;
    t->b_see_sum = 0ull;
    t->n_see_sum = 0ull;
    t->b_see_max = 0;
    t->b_see_min = 0xffffffff;
    for (int i = 0; i < TAY_MAX_THREADS; ++i) {
        TayThreadContext *ctx = &runner.threads[i].context;
        ctx->broad_see_phase = 0;
        ctx->narrow_see_phase = 0;
    }
    for (int i = 0; i < TAY_TELEMETRY_MAX_SAMPLES; ++i)
        t->b_see_samples[i] = 0;
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
    _reset_telemetry();
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

void tay_threads_update_telemetry() {
    TayTelemetry *t = &runner.telemetry;

    for (int i = 0; i < runner.count; ++i) {
        TayThreadContext *c = &runner.threads[i].context;
        t->b_see_sum += c->broad_see_phase;
        t->n_see_sum += c->narrow_see_phase;
        if (c->broad_see_phase > t->b_see_max)
            t->b_see_max = c->broad_see_phase;
        if (c->broad_see_phase < t->b_see_min)
            t->b_see_min = c->broad_see_phase;
        if (t->samples_count < TAY_TELEMETRY_MAX_SAMPLES) {
            t->b_see_samples[t->samples_count] = c->broad_see_phase;
            ++t->samples_count;
        }
        else {
            int sample = rand() % TAY_TELEMETRY_MAX_SAMPLES;
            t->b_see_samples[sample] = c->broad_see_phase;
        }
        c->broad_see_phase = 0;
        c->narrow_see_phase = 0;
        ++t->thread_runs_count;
    }

    ++t->steps_count;
}

static double _max(double a, double b) {
    return (a > b) ? a : b;
}

void tay_threads_report_telemetry(int steps_between_reports) {
    TayTelemetry *t = &runner.telemetry;
    if (t->steps_count % steps_between_reports)
        return;

    double b_see_mean = t->b_see_sum / (double)t->thread_runs_count;
    double b_see_deviation_mean = 0.0;
    for (int i = 0; i < t->samples_count; ++i)
        b_see_deviation_mean += fabs((double)t->b_see_samples[i] - b_see_mean);
    b_see_deviation_mean /= (double)t->samples_count;
    double b_see_deviation_max = _max(fabs((double)t->b_see_max - b_see_mean), fabs((double)t->b_see_min - b_see_mean));

    printf("Thread see phase balancing (potential see interactions per thread per step):\n");
    printf(" -relative deviation mean: %.2f%%\n", b_see_deviation_mean * 100.0 / b_see_mean);
    printf(" -trelative deviation max: %.2f%%\n", b_see_deviation_max * 100.0 / b_see_mean);
    printf("See interaction culling efficiency (actual / potential): %.2f%%\n", t->n_see_sum * 100.0 / (double)t->b_see_sum);
    printf("Mean actual see interactions per step: %.2f\n", t->n_see_sum / (double)t->steps_count);

    _reset_telemetry();
}
