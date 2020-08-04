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

static void _plain_destroy(TaySpace *space) {
    free(space->storage);
}

static void _plain_add(TaySpace *space, TayAgent *agent, int group) {
    Plain *p = space->storage;
    PlainGroup *g = p->groups + group;
    int thread = (g->receiving_thread++) % runner.count;
    TayAgent *next = g->first[thread];
    agent->prev = 0;
    agent->next = next;
    if (next)
        next->prev = agent;
    g->first[thread] = agent;
}

static void _plain_perception(TaySpace *space, int group1, int group2, void (*func)(void *, void *, void *), void *context) {
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

static void _plain_action(TaySpace *space, int group, void(*func)(void *, void *), void *context) {
    Plain *p = space->storage;
    for (int i = 0; i < runner.count; ++i)
        tay_thread_set_action(i, context, func, p->groups[group].first[i], 0);
    tay_runner_run();
}

static void _plain_iter(TaySpace *space, int group, void (*func)(void *)) {
    Plain *p = space->storage;
    for (int i = 0; i < runner.count; ++i)
        for (TayAgent *a = p->groups[group].first[i]; a; a = a->next)
            func(a);
}

void space_plain_init(TaySpace *space) {
    Plain *p = calloc(1, sizeof(Plain));
    space->storage = p;
    space->destroy = _plain_destroy;
    space->add = _plain_add;
    space->perception = _plain_perception;
    space->action = _plain_action;
    space->iter = _plain_iter;
}
