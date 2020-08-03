#include "test.h"
#include "state.h"
#include "vec.h"
#include <stdlib.h>


typedef struct {
    TayAgent agent;
    /* movement variables */
    vec p, v;
    /* control variables */
    vec acc, hea;
    int acc_count;
} Agent;

typedef struct {
    float perception_r;
    float space_r;
    int perception_calls;
    int actual_perception_calls;
} Context;

static void _agent_perception_actual(Agent *a, vec d, float l, Context *c) {
    vec n = vec_div_scalar(d, l);
    a->acc = vec_add(a->acc, n);
    ++a->acc_count;
    ++c->actual_perception_calls;
}

/* NOTE: only b agent can write to itself, can only read from a */
static void _agent_perception(Agent *a, Agent *b, Context *c) {
    vec d = vec_sub(b->p, a->p);
    float l = vec_length(d);
    if (l < c->perception_r)
        _agent_perception_actual(a, d, l, c);
    ++c->perception_calls;
}

static void _agent_action(Agent *a, Context *c) {
    if (a->acc_count) {
        a->hea = vec_normalize(vec_add(a->hea, vec_normalize_to(a->acc, 0.2f)));
        a->acc = vec_null();
        a->acc_count = 0;
    }
    a->p = vec_add(a->p, a->v);
    if (a->p.x <= -c->space_r) {
        a->v.x = -a->v.x;
        a->p.x = -c->space_r;
    }
    if (a->p.y <= -c->space_r) {
        a->v.y = -a->v.y;
        a->p.y = -c->space_r;
    }
    if (a->p.z <= -c->space_r) {
        a->v.z = -a->v.z;
        a->p.z = -c->space_r;
    }
    if (a->p.x >= c->space_r) {
        a->v.x = -a->v.x;
        a->p.x = c->space_r;
    }
    if (a->p.y >= c->space_r) {
        a->v.y = -a->v.y;
        a->p.y = c->space_r;
    }
    if (a->p.z >= c->space_r) {
        a->v.z = -a->v.z;
        a->p.z = c->space_r;
    }
}

static void _make_cluster(TayState *state, int group, int count, vec min, vec max) {
    for (int i = 0; i < count; ++i) {
        Agent *a = tay_alloc_agent(state, group);
        a->p = vec_rand(min, max);
        a->v = vec_rand_scalar(-1.0f, 1.0f);
        a->v = vec_normalize(a->v);
        a->acc = vec_null();
        a->acc_count = 0;
        a->hea = vec_make(1.0f, 0.0f, 0.0f);
    }
}

static void _inspect(Agent *a, Context *c) {
    int brk = 0;
}

static void _test_case_one_group(int count) {
    TayState *s = tay_create_state(TAY_SPACE_PLAIN);

    int g = tay_add_group(s, sizeof(Agent), 1000000);
    tay_add_perception(s, g, g, _agent_perception);
    tay_add_action(s, g, _agent_action);

    Context context = { 50.0f, 100.0f, 0, 0 };

    _make_cluster(s, g, count,
                  vec_make(-context.space_r, -context.space_r, -context.space_r),
                  vec_make(context.space_r, context.space_r, context.space_r));

    tay_run(s, 10, &context);
    _inspect(s->groups[g].storage, &context);

    tay_destroy_state(s);
}

void test() {
    srand(1);
    _test_case_one_group(2000);
}
