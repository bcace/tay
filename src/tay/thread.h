#ifndef tay_thread_h
#define tay_thread_h

#define TAY_MAX_THREADS 32


typedef void *Handle;

typedef struct TayThreadContext {
    void *context;
    int all_see_runs;
    int useful_see_runs;
} TayThreadContext;

typedef struct TayThread {
    TayThreadContext context;
    struct TayAgent *beg_a; /* seer agents */
    struct TayAgent *beg_b; /* seen agents */
    struct TayPass *pass;
    int dims; /* needed for see only */
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
void tay_runner_clear_stats();
void tay_runner_start_threads(int threads_count);
void tay_thread_set_see(int index, void *context,
                               struct TayAgent *beg_a, struct TayAgent *beg_b,
                               struct TayPass *pass,
                               int dims);
void tay_thread_set_act(int index, void *context,
                           struct TayAgent *beg_a,
                           struct TayPass *pass);
void tay_runner_run();
void tay_runner_stop_threads();
void tay_runner_run_no_threads();

extern TayRunner runner;

#endif
