#include "test.h"
#include "tay.h"
#include "vec.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


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

/* Agent a is the seer (writes to itself) and agent b is the seen agent (is only read from),
this should be somehow statically checked, so that the actual code doesn't do the wrong thing. */
static void _agent_see(Agent *a, Agent *b, void *context) {
    vec d = vec_sub(b->p, a->p);
    float l = vec_length(d);
    Context *c = context;
    if (l < c->perception_r)
        _agent_perception_actual(a, d, l, c);
}

static void _agent_act(Agent *a, void *context) {
    Context *c = context;
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

static void _make_cluster(TayState *state, int group, int count, float radius, float speed, float space_radius) {
    vec cluster_p = vec_rand_scalar(-space_radius, space_radius);
    vec cluster_v = vec_rand_scalar(-1.0f, 1.0f);
    cluster_v = vec_normalize_to(cluster_v, speed);
    for (int i = 0; i < count; ++i) {
        Agent *a = tay_get_new_agent(state, group);
        a->cluster_p = cluster_p;
        a->p = vec_add(cluster_p, vec_rand_scalar(-radius, radius));
        a->v = cluster_v;
        a->acc = vec_null();
        a->acc_count = 0;
        a->hea = vec_make(1.0f, 0.0f, 0.0f);
        tay_add_new_agent(state, group);
    }
}

static inline void _eq(float a, float b) {
    float c = a - b;
    static float epsilon = 0.0001f;
    assert(c > -epsilon && c < epsilon);
}

typedef enum {
    RA_NONE,
    RA_WRITE,
    RA_COMPARE,
} ResultsAction;

typedef struct {
    vec data[100000];
    int first_time;
} Results;

static Results *_create_results() {
    Results *r = malloc(sizeof(Results));
    r->first_time = 1;
    return r;
}

static void _destroy_results(Results *r) {
    free(r);
}

/* TODO: describe model case */
static void _test_model_case1(TaySpaceType space_type, float perception_r, int max_depth_correction, Results *results) {
    int agents_count = 4000;
    float space_r = 100.0f;

    srand(1);

    Context context;
    context.perception_r = perception_r;
    context.space_r = space_r;
    float radii[] = { perception_r, perception_r, perception_r };

    TayState *s = tay_create_state(space_type, 3, radii, max_depth_correction);

    int g = tay_add_group(s, sizeof(Agent), agents_count);
    tay_add_see(s, g, g, _agent_see, radii);
    tay_add_act(s, g, _agent_act);

    for (int i = 0; i < agents_count; ++i)
        _make_cluster(s, g, 1, space_r, 1.0f, space_r);

    printf("R: %g, depth_correction: %d\n", perception_r, max_depth_correction);

    tay_run(s, 100, &context);

    if (results) {
        if (results->first_time) {
            Agent *agents = tay_get_storage(s, g);
            for (int i = 0; i < agents_count; ++i)
                results->data[i] = agents[i].hea;
            results->first_time = 0;
        }
        else {
            Agent *agents = tay_get_storage(s, g);
            for (int i = 0; i < agents_count; ++i) {
                vec a = agents[i].hea;
                vec b = results->data[i];
                _eq(a.x, b.x);
                _eq(a.y, b.y);
                _eq(a.z, b.z);
            }
        }
    }

    tay_destroy_state(s);
}

void test() {

#if 1
    Results *r = _create_results();
#else
    Results *r = 0;
#endif

    /* testing model case 1 */

    for (int i = 2; i < 3; ++i) {
        float perception_r = 10.0f * (1 << i);

        for (int j = 0; j < 4; ++j)
            _test_model_case1(TAY_SPACE_TREE, perception_r, j, r);

        printf("reference:\n");

        _test_model_case1(TAY_SPACE_SIMPLE, perception_r, 0, r);

        printf("\n");
    }

    _destroy_results(r);
}
