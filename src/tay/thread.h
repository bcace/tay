#ifndef tay_thread_h
#define tay_thread_h

#include "const.h"


typedef void * Handle;

typedef struct TayThreadContext {
    void *context; /* model context */
    int broad_see_phase;
    int narrow_see_phase;
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

typedef struct TayRunner {
    TayThread threads[TAY_MAX_THREADS];
    int count;
    TayRunnerState state;
} TayRunner;

void tay_runner_init();
void tay_runner_start_threads(int threads_count);
void tay_thread_set_task(int index, void (*task_func)(void *, TayThreadContext *), void *task, void *context);
void tay_runner_run();
void tay_runner_stop_threads();
void tay_runner_run_no_threads();
void tay_runner_reset_stats();
void tay_runner_report_stats();

extern TayRunner runner;

#endif
