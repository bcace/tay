#ifndef tay_thread_h
#define tay_thread_h

#define TAY_MAX_THREADS 64


typedef void *Handle;

// typedef struct TayThreadContext {
//     void *context;
//     int all_see_runs;
//     int useful_see_runs;
// } TayThreadContext;

// typedef struct TayThreadSeeTask {
//     struct TayAgent *seer_agents;
//     union {
//         struct TayAgent *seen_agents; /* if single seen */
//         struct TayAgent **seen_bundles; /* if multiple seen */
//     };
//     int seen_bundles_count; /* -1 if single seen */
// } TayThreadSeeTask;

typedef struct TayThread {
    // TayThreadContext context;
    void *task_context; /* not the context that gets passed to the actual agent functions, but this context should contain that one */
    void (*task_func)(void *task_context); /* not agent function, but it calls the agent function */
    // union {
    //     struct TayAgent *act_agents; /* if act */
    //     struct { /* if see */
    //         union {
    //             TayThreadSeeTask see_task; /* if single see task */
    //             TayThreadSeeTask *see_tasks; /* if multiple see tasks */
    //         };
    //         int see_tasks_count; /* -1 if single see task */
    //     };
    // };
    // struct TayPass *pass;
    // int dims; /* needed for see only */

    int run;
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
// void tay_runner_clear_stats();
void tay_runner_start_threads(int threads_count);
void tay_thread_set_task(int index, void (*task_func)(void *), void *task_context);
void tay_runner_run();
void tay_runner_stop_threads();
void tay_runner_run_no_threads();

extern TayRunner runner;

#endif
