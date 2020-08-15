#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>


typedef struct {
    TayAgent *first[TAY_MAX_THREADS];
    int receiving_thread;
} PlainGroup;

typedef struct {
    PlainGroup groups[TAY_MAX_GROUPS];
} Plain;

static void _destroy(TaySpace *space) {
    free(space->storage);
}

static void _add(TaySpace *space, TayAgent *agent, int group) {
    Plain *p = space->storage;
    PlainGroup *g = p->groups + group;
    int thread = (g->receiving_thread++) % runner.count;
    TayAgent *next = g->first[thread];
    agent->next = next;
    g->first[thread] = agent;
}

static void _perception(TaySpace *space, int group1, int group2, void (*func)(void *, void *, void *), void *context) {
    Plain *p = space->storage;
    for (int i = 0; i < runner.count; ++i) {
        TayAgent *b = p->groups[group1].first[i];
        for (int j = 0; j < runner.count; ++j) {
            TayAgent *a = p->groups[group2].first[j];
            tay_thread_set_perception(j, context, func, a, 0, b, 0);
        }
        tay_runner_run();
    }
    if (group1 != group2) {
        for (int i = 0; i < runner.count; ++i) {
            TayAgent *b = p->groups[group2].first[i];
            for (int j = 0; j < runner.count; ++j) {
                TayAgent *a = p->groups[group1].first[j];
                tay_thread_set_perception(j, context, func, a, 0, b, 0);
            }
            tay_runner_run();
        }
    }
}

static void _action(TaySpace *space, int group, void(*func)(void *, void *), void *context) {
    Plain *p = space->storage;
    for (int i = 0; i < runner.count; ++i)
        tay_thread_set_action(i, context, func, p->groups[group].first[i], 0);
    tay_runner_run();
}

static void _post(TaySpace *space, void (*func)(void *), void *context) {
    Plain *p = space->storage;
    func(context);
}

static void _iter(TaySpace *space, int group, void (*func)(void *, void *), void *context) {
    Plain *p = space->storage;
    for (int i = 0; i < runner.count; ++i)
        for (TayAgent *a = p->groups[group].first[i]; a; a = a->next)
            func(a, context);
}

static void _update(TaySpace *space) {
}

void space_plain_init(TaySpace *space) {
    Plain *p = calloc(1, sizeof(Plain));
    space_init(space, p, _destroy, _add, _perception, _action, _post, _iter, _update);
}
