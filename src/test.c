#include "test.h"
#include "tay.h"
#include "vec.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>


#pragma pack(push, 1)
typedef struct {
    /* position variables (must be first, must be collection of floats) */
    vec p;
    vec v;
    /* control variables */
    vec b_buffer;
    int b_buffer_count;
    vec f_buffer;
} Agent;

typedef struct {
    vec min;
    vec max;
} ActContext;

typedef struct {
    vec radii;
} SeeContext;
#pragma pack(pop)

/* Agent a is the seer (writes to itself) and agent b is the seen agent (is only read from),
this should be somehow statically checked, so that the actual code doesn't do the wrong thing. */
static void _agent_see(Agent *a, Agent *b, void *context) {
    a->b_buffer = vec_add(a->b_buffer, vec_sub(b->p, a->p));
    a->b_buffer_count++;
}

static void _agent_act(Agent *agent, void *context) {
    ActContext *c = context;

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        agent->f_buffer = vec_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->b_buffer = vec_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    agent->p = vec_add(agent->p, agent->v);

    if (agent->p.x < c->min.x) {
        agent->p.x = c->min.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->p.y < c->min.y) {
        agent->p.y = c->min.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->p.z < c->min.z) {
        agent->p.z = c->min.z;
        agent->v.z = -agent->v.z;
    }

    if (agent->p.x > c->max.x) {
        agent->p.x = c->max.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->p.y > c->max.y) {
        agent->p.y = c->max.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->p.z > c->max.z) {
        agent->p.z = c->max.z;
        agent->v.z = -agent->v.z;
    }
}

/* Note that this only makes agents with randomized directions. Should implement another
one for all agents with same direction later. */
static void _make_cluster(TayState *state, int group, int count, vec min, vec max, float velocity) {
    for (int i = 0; i < count; ++i) {
        Agent *a = tay_get_available_agent(state, group);
        a->p.x = min.x + rand() * (max.x - min.x) / (float)RAND_MAX;
        a->p.y = min.y + rand() * (max.y - min.y) / (float)RAND_MAX;
        a->p.z = min.z + rand() * (max.z - min.z) / (float)RAND_MAX;
        a->v.x = -0.5f + rand() / (float)RAND_MAX;
        a->v.y = -0.5f + rand() / (float)RAND_MAX;
        a->v.z = -0.5f + rand() / (float)RAND_MAX;
        float l = velocity / sqrtf(a->v.x * a->v.x + a->v.y * a->v.y + a->v.z * a->v.z);
        a->v.x *= l;
        a->v.y *= l;
        a->v.z *= l;
        a->b_buffer.x = 0.0f;
        a->b_buffer.y = 0.0f;
        a->b_buffer.z = 0.0f;
        a->b_buffer_count = 0;
        a->f_buffer.x = 0.0f;
        a->f_buffer.y = 0.0f;
        a->f_buffer.z = 0.0f;
        tay_commit_available_agent(state, group);
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

static void _reset_results(Results *r) {
    if (r)
        r->first_time = 1;
}

static void _destroy_results(Results *r) {
    if (r)
        free(r);
}

/* TODO: describe model case */
static void _test_model_case1(TaySpaceType space_type, float see_radius, int max_depth_correction, Results *results) {
    int dims = 3;
    int agents_count = 10000;
    float space_size = 200.0f;

    srand(1);

    float see_radii[] = { see_radius, see_radius, see_radius };

    TayState *s = tay_create_state(space_type, dims, see_radii, max_depth_correction);

    ActContext act_context;
    act_context.min.x = 0.0f;
    act_context.min.y = 0.0f;
    act_context.min.z = 0.0f;
    act_context.max.x = space_size;
    act_context.max.y = space_size;
    act_context.max.z = space_size;

    SeeContext see_context;
    see_context.radii.x = see_radius;
    see_context.radii.y = see_radius;
    see_context.radii.z = see_radius;

    int g = tay_add_group(s, sizeof(Agent), agents_count);
    tay_add_see(s, g, g, _agent_see, see_radii, &see_context);
    tay_add_act(s, g, _agent_act, &act_context);

    _make_cluster(s, g, agents_count, vec_make(0.0f, 0.0f, 0.0f), vec_make(space_size, space_size, space_size), 1.0f);

    printf("R: %g, depth_correction: %d\n", see_radius, max_depth_correction);

    tay_run(s, 100);

    if (results) {
        if (results->first_time) {
            for (int i = 0; i < agents_count; ++i) {
                Agent *agent = tay_get_agent(s, g, i);
                results->data[i] = agent->f_buffer;
            }
            results->first_time = 0;
        }
        else {
            for (int i = 0; i < agents_count; ++i) {
                Agent *agent = tay_get_agent(s, g, i);
                vec a = agent->f_buffer;
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

    for (int i = 0; i < 3; ++i) {
        float perception_r = 10.0f * (1 << i);

#if 0
        for (int j = 0; j < 4; ++j)
            _test_model_case1(TAY_SPACE_TREE, perception_r, j, r);
#endif

#if 1
        printf("gpu:\n");

        _test_model_case1(TAY_SPACE_GPU_SIMPLE, perception_r, 0, r);
#endif

#if 0
        printf("reference:\n");

        _test_model_case1(TAY_SPACE_SIMPLE, perception_r, 0, r);
#endif

        printf("\n");

        _reset_results(r);
    }

    _destroy_results(r);
}
