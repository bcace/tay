#ifndef tay_thread_h
#define tay_thread_h

#include "const.h"


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
    double rel_dev_mean_sum;
    double rel_dev_max_sum;
    double rel_dev_max;
    int steps_count;
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
void tay_threads_report_telemetry(unsigned steps_between_reports);

extern TayRunner runner;

#endif