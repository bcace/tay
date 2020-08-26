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

static void _perception(TaySpace *space, TayPass *pass, void *context) {
    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i) {
        TayAgent *b = s->groups[pass->group1].first[i];
        for (int j = 0; j < runner.count; ++j) {
            TayAgent *a = s->groups[pass->group2].first[j];
            tay_thread_set_perception(j, context, pass->perception, a, 0, b, 0);
        }
        tay_runner_run();
    }
    if (pass->group1 != pass->group2) {
        for (int i = 0; i < runner.count; ++i) {
            TayAgent *b = s->groups[pass->group2].first[i];
            for (int j = 0; j < runner.count; ++j) {
                TayAgent *a = s->groups[pass->group1].first[j];
                tay_thread_set_perception(j, context, pass->perception, a, 0, b, 0);
            }
            tay_runner_run();
        }
    }
}

static void _action(TaySpace *space, TayPass *pass, void *context) {
    Simple *s = space->storage;
    for (int i = 0; i < runner.count; ++i)
        tay_thread_set_action(i, context, pass->action, s->groups[pass->group1].first[i], 0);
    tay_runner_run();
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

void space_simple_init(TaySpace *space) {
    Simple *s = calloc(1, sizeof(Simple));
    space_init(space, s, _destroy, _add, _perception, _action, _post, _iter, _update);
}
