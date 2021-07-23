#ifndef tay_thread_h
#define tay_thread_h

#include "config.h"


typedef void * Handle;

typedef struct TayThreadContext {
    void *context; /* model context */
    void *storage;
    unsigned storage_size;
#if TAY_TELEMETRY
    // TODO: make names more similar to ones in telemetry structures
    unsigned broad_see_phase;   /* single simulation step only */
    unsigned narrow_see_phase;  /* single simulation step only */
    unsigned long long grid_seer_bins;              /* single simulation step only */
    unsigned long long grid_seer_kernel_rebuilds;   /* single simulation step only */
#endif
} TayThreadContext;

typedef struct TayThreadTask {
    struct TayPass *pass;
    int thread_i;
} TayThreadTask;

typedef struct TayThread {
    TayThreadTask *task;
    void (*task_func)(TayThreadTask *task, TayThreadContext *thread_context);
    TayThreadContext context;
    int run; /* signal that the thread function should end */
    Handle thread;
    Handle beg_semaphore;
    Handle end_semaphore;
} TayThread;

typedef enum TayRunnerState {
    TAY_RUNNER_IDLE,    /* no threads started */
    TAY_RUNNER_RUNNING, /* threads started and working on tasks */
    TAY_RUNNER_WAITING, /* threads started but waiting for tasks */
} TayRunnerState;

typedef struct {
    unsigned long long b_see_sum;
    unsigned long long n_see_sum;
    double rel_dev_mean_sum;
    double rel_dev_max_sum;
    double rel_dev_max;
    unsigned long long g_seer_bins;
    unsigned long long g_seer_kernel_rebuilds;
    unsigned long long tree_agents;
    unsigned long long tree_branch_agents;
    int steps_count;
} TayTelemetry;

typedef struct {
    double mean_relative_deviation_averaged;
    double max_relative_deviation_averaged;
    double max_relative_deviation;
    double see_culling_efficiency;
    double mean_see_interactions_per_step;
    double grid_kernel_rebuilds;
    double tree_branch_agents;
} TayTelemetryResults;

typedef struct TayRunner {
    TayThread threads[TAY_MAX_THREADS];
    int count;
    TayRunnerState state;
    TayTelemetry telemetry;
    unsigned thread_storage_size;
} TayRunner;

typedef struct {
    unsigned beg;
    unsigned end;
} TayRange;

void tay_thread_set_task(int index, void (*task_func)(TayThreadTask *, TayThreadContext *), TayThreadTask *task, void *context);
void tay_runner_run();
void tay_runner_run_no_threads();

void tay_threads_update_telemetry();
void tay_threads_report_telemetry(unsigned steps_between_reports, void *file);
void tay_threads_get_telemetry_results(TayTelemetryResults *telemetry_results);

TayRange tay_threads_range(unsigned count, unsigned thread_i);

extern TayRunner runner;

#endif
