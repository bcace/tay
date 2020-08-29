#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>


typedef struct {
    TayAgent *first[TAY_MAX_THREADS];
    int receiving_thread;
} SimpleGroup;

typedef struct {
    SimpleGroup groups[TAY_MAX_GROUPS];
} Simple;

static void _destroy(TaySpace *space) {
    free(space->storage);
}

static void _add(TaySpace *space, TayAgent *agent, int group) {
    Simple *s = space->storage;
    SimpleGroup *g = s->groups + group;
    int thread = (g->receiving_thread++) % runner.count;
    TayAgent *next = g->first[thread];
    agent->next = next;
    g->first[thread] = agent;
}

static void _see(TaySpace *space, TayPass *pass, void *context) {
    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i) {
        TayAgent *b = s->groups[pass->seen_group].first[i];
        for (int j = 0; j < runner.count; ++j) {
            TayAgent *a = s->groups[pass->seer_group].first[j];
            tay_thread_set_see(j, context, a, b, pass, space->dims);
        }
        tay_runner_run_no_threads();
    }
}

static void _act(TaySpace *space, TayPass *pass, void *context) {
    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i)
        tay_thread_set_act(i, context, s->groups[pass->act_group].first[i], pass);
    tay_runner_run_no_threads();
}

static void _post(TaySpace *space, void (*func)(void *), void *context) {
    Simple *s = space->storage;
    func(context);
}

static void _iter(TaySpace *space, int group, void (*func)(void *, void *), void *context) {
    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i)
        for (TayAgent *a = s->groups[group].first[i]; a; a = a->next)
            func(a, context);
}

static void _update(TaySpace *space) {
}

void space_simple_init(TaySpace *space, int dims) {
    Simple *s = calloc(1, sizeof(Simple));
    space_init(space, s, dims, _destroy, _add, _see, _act, _post, _iter, _update);
}
