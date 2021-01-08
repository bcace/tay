#ifndef tay_thread_h
#define tay_thread_h

#include "const.h"

#define TAY_TELEMETRY_MAX_SAMPLES 100


typedef void * Handle;

typedef struct TayThreadContext {
    void *context; /* model context */
    unsigned broad_see_phase;   /* single simulation step only */
    unsigned narrow_see_phase;  /* single simulation step only */
} TayThreadContext;

typedef struct TayThread {
    void *task;
    void (*task_func)(void *task, TayThreadContext *thread_context);
    TayThreadContext context;
    int run; /* signal that the thread function should end */
    Handle thread;
    Handle beg_semaphore;
    Handle end_semaphore;
} TayThread;

typedef enum TayRunnerState {
    TAY_RUNNER_IDLE,
    TAY_RUNNER_RUNNING,
    TAY_RUNNER_WAITING,
} TayRunnerState;

typedef struct {
    unsigned long long b_see_sum;
    unsigned long long n_see_sum;
    unsigned b_see_max, b_see_min;
    unsigned n_see_max, n_see_min;
    unsigned b_see_samples[TAY_TELEMETRY_MAX_SAMPLES];
    unsigned n_see_samples[TAY_TELEMETRY_MAX_SAMPLES];
    int thread_runs_count; // TODO: rename! this is thread runs count or something...
    int samples_count;
} TayTelemetry;

typedef struct TayRunner {
    TayThread threads[TAY_MAX_THREADS];
    int count;
    TayRunnerState state;
    TayTelemetry telemetry;
} TayRunner;

void tay_runner_init();
void tay_runner_start_threads(int threads_count);
void tay_thread_set_task(int index, void (*task_func)(void *, TayThreadContext *), void *task, void *context);
void tay_runner_run();
void tay_runner_stop_threads();
void tay_runner_run_no_threads();

void tay_threads_update_telemetry();
void tay_threads_report_telemetry();

extern TayRunner runner;

#endif
