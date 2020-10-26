#include "test.h"
#include "tay.h"
#include "agent.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>


/* Note that this only makes agents with randomized directions. Should implement another
one for all agents with same direction later. */
static void _make_cluster(TayState *state, int group, int count, float4 min, float4 max, float velocity) {
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
    static float epsilon = 0.001f;
    if (c < -epsilon || c > epsilon) {
        fprintf(stderr, "result error\n");
        assert(0);
    }
}

typedef enum {
    RA_NONE,
    RA_WRITE,
    RA_COMPARE,
} ResultsAction;

typedef struct {
    float4 data[10000000];
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
    int agents_count = 4000;
    float space_size = 200.0f;

    srand(1);

    float see_radii[] = { see_radius, see_radius, see_radius };

    TayState *s = tay_create_state(space_type, dims, see_radii, max_depth_correction);
    tay_set_source(s, agent_kernels_source);

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
    tay_add_see(s, g, g, agent_see, "agent_see", see_radii, &see_context, sizeof(see_context));
    tay_add_act(s, g, agent_act, "agent_act", &act_context, sizeof(act_context));

    _make_cluster(s, g, agents_count, float4_make(0.0f, 0.0f, 0.0f), float4_make(space_size, space_size, space_size), 1.0f);

    printf("R: %g, depth_correction: %d\n", see_radius, max_depth_correction);

    tay_simulation_start(s);
    tay_run(s, 1000);
    tay_simulation_end(s);

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
                float4 a = agent->f_buffer;
                float4 b = results->data[i];
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

    int beg_depth_correction = 0;
    int end_depth_correction = 1;

    /* testing model case 1 */

    for (int i = 0; i < 3; ++i) {
        float perception_r = 10.0f * (1 << i);

#if 0
        for (int j = beg_depth_correction; j < end_depth_correction; ++j)
            _test_model_case1(TAY_SPACE_TREE, perception_r, j, r);
#endif

#if 0
        printf("reference:\n");
        _test_model_case1(TAY_SPACE_SIMPLE, perception_r, 0, r);
#endif

#if 0
        printf("gpu:\n");
        _test_model_case1(TAY_SPACE_GPU_SIMPLE, perception_r, 0, r);
#endif

#if 1
        printf("gpu tree:\n");
        for (int j = beg_depth_correction; j < end_depth_correction; ++j)
            _test_model_case1(TAY_SPACE_GPU_TREE, perception_r, j, r);
#endif

        printf("\n");

        _reset_results(r);
    }

    _destroy_results(r);
}
