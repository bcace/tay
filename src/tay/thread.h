#ifndef tay_thread_h
#define tay_thread_h

#define TAY_MAX_THREADS 32


typedef void *Handle;

typedef struct TayThread {
    void *context;
    struct TayAgent *beg_a, *end_a; /* subjects */
    struct TayAgent *beg_b, *end_b; /* objects */
    void (*perception)(struct TayAgent *a, struct TayAgent *b, void *context);
    void (*action)(struct TayAgent *a, void *context);
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
void tay_runner_start_threads(int threads_count);
void tay_thread_set_perception(int index, void *context,
                               void (*func)(struct TayAgent *a, struct TayAgent *b, void *context),
                               struct TayAgent *beg_a, struct TayAgent *end_a,
                               struct TayAgent *beg_b, struct TayAgent *end_b);
void tay_thread_set_action(int index, void *context,
                           void (*func)(struct TayAgent *a, void *context),
                           struct TayAgent *beg_a, struct TayAgent *end_a);
void tay_runner_run();
void tay_runner_stop_threads();

extern TayRunner runner;

#endif
