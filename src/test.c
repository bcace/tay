#include "test.h"
#include "tay.h"
#include "vec.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>


typedef struct {
    TayAgent base;
    /* position variables (must be first, must be collection of floats) */
    vec p;
    /* movement variables */
    vec v, cluster_p;
    /* control variables */
    vec acc, hea;
    int acc_count;
} Agent;

typedef struct {
    float perception_r;
    float space_r;
} Context;

static void _agent_perception_actual(Agent *a, vec d, float l, Context *c) {
    vec n = vec_div_scalar(d, l);
    a->acc = vec_add(a->acc, n);
    ++a->acc_count;
}

/* NOTE: only b agent can write to itself, can only read from a */
static void _agent_perception(Agent *a, Agent *b, Context *c) {
    vec d = vec_sub(b->p, a->p);
    float l = vec_length(d);
    if (l < c->perception_r)
        _agent_perception_actual(a, d, l, c);
}

static void _agent_action(Agent *a, Context *c) {
    if (a->acc_count) {
        a->hea = vec_normalize(vec_add(a->hea, vec_normalize_to(a->acc, 0.2f)));
        a->acc = vec_null();
        a->acc_count = 0;
    }
    a->p = vec_add(a->p, a->v);
    a->cluster_p = vec_add(a->cluster_p, a->v);
    if (a->cluster_p.x < -c->space_r) {
        a->p.x -= c->space_r + a->cluster_p.x;
        a->cluster_p.x = -c->space_r;
        a->v.x = -a->v.x;
    }
    if (a->cluster_p.y < -c->space_r) {
        a->p.y -= c->space_r + a->cluster_p.y;
        a->cluster_p.y = -c->space_r;
        a->v.y = -a->v.y;
    }
    if (a->cluster_p.z < -c->space_r) {
        a->p.z -= c->space_r + a->cluster_p.z;
        a->cluster_p.z = -c->space_r;
        a->v.z = -a->v.z;
    }
    if (a->cluster_p.x > c->space_r) {
        a->p.x += c->space_r - a->cluster_p.x;
        a->cluster_p.x = c->space_r;
        a->v.x = -a->v.x;
    }
    if (a->cluster_p.y > c->space_r) {
        a->p.y += c->space_r - a->cluster_p.y;
        a->cluster_p.y = c->space_r;
        a->v.y = -a->v.y;
    }
    if (a->cluster_p.z > c->space_r) {
        a->p.z += c->space_r - a->cluster_p.z;
        a->cluster_p.z = c->space_r;
        a->v.z = -a->v.z;
    }
}

static void _make_cluster(struct TayState *state, int group, int count, float radius, float speed, Context *context) {
    vec cluster_p = vec_rand_scalar(-context->space_r, context->space_r);
    vec cluster_v = vec_rand_scalar(-1.0f, 1.0f);
    cluster_v = vec_normalize_to(cluster_v, speed);
    for (int i = 0; i < count; ++i) {
        Agent *a = tay_alloc_agent(state, group);
        a->cluster_p = cluster_p;
        a->p = vec_add(cluster_p, vec_rand_scalar(-radius, radius));
        a->v = cluster_v;
        a->acc = vec_null();
        a->acc_count = 0;
        a->hea = vec_make(1.0f, 0.0f, 0.0f);
    }
}

static void _inspect_agent(Agent *a, Context *context) {
    printf("%g\n", a->p.x);
}

static void _setup0(struct TayState *state, Context *context) {
    context->perception_r = 50.0f;
    context->space_r = 100.0f;

    int g = tay_add_group(state, sizeof(Agent), 1000000);
    tay_add_perception(state, g, g, _agent_perception);
    tay_add_action(state, g, _agent_action);

    for (int i = 0; i < 2000; ++i)
        _make_cluster(state, g, 1, 10, 1.0f, context);
}

static void _setup1(struct TayState *state, Context *context) {
    context->perception_r = 50.0f;
    context->space_r = 100.0f;

    int g = tay_add_group(state, sizeof(Agent), 1000000);
    tay_add_perception(state, g, g, _agent_perception);
    tay_add_action(state, g, _agent_action);

    _make_cluster(state, g, 2000, 10, 1.0f, context);
}

static void _test(void (*setup)(struct TayState *, Context *)) {
    float diameters[] = { 10.0f, 10.0f, 10.0f };
    struct TayState *s = tay_create_state(TAY_SPACE_SIMPLE, 3, diameters);

    srand(1);
    Context context;
    setup(s, &context);

    tay_run(s, 100, &context);

    _inspect_agent(tay_get_storage(s, 0), &context);

    tay_destroy_state(s);
}

void test() {
    _test(_setup1);
}
