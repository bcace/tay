#include "test.h"
#include "tay.h"
#include "agent.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>


static void _make_randomized_direction_cluster(TayState *state, int group, int count, float3 min, float3 max, float velocity) {
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

static void _make_uniform_direction_cluster(TayState *state, int group, int count, float3 min, float3 max, float velocity) {
    float3 v;
    v.x = -0.5f + rand() / (float)RAND_MAX;
    v.y = -0.5f + rand() / (float)RAND_MAX;
    v.z = -0.5f + rand() / (float)RAND_MAX;
    float l = velocity / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    v.x *= l;
    v.y *= l;
    v.z *= l;
    for (int i = 0; i < count; ++i) {
        Agent *a = tay_get_available_agent(state, group);
        a->p.x = min.x + rand() * (max.x - min.x) / (float)RAND_MAX;
        a->p.y = min.y + rand() * (max.y - min.y) / (float)RAND_MAX;
        a->p.z = min.z + rand() * (max.z - min.z) / (float)RAND_MAX;
        a->v.x = v.x;
        a->v.y = v.y;
        a->v.z = v.z;
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

static inline void _check_error(float a, float b, float *max_error) {
    float relative_error = (a - b) / a;
    if (relative_error > *max_error)
        *max_error = relative_error;
    else if (-relative_error > *max_error)
        *max_error = -relative_error;
}

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

static void _write_or_compare_results(Results *results, TayState *tay, int group, int agents_count) {
    if (results) {
        if (results->first_time) {
            for (int i = 0; i < agents_count; ++i) {
                Agent *agent = tay_get_agent(tay, group, i);
                results->data[i] = agent->f_buffer;
            }
            results->first_time = 0;
        }
        else {
            float max_error = 0.0f;

            for (int i = 0; i < agents_count; ++i) {
                Agent *agent = tay_get_agent(tay, group, i);
                float4 a = results->data[i];
                float4 b = agent->f_buffer;
                _check_error(a.x, b.x, &max_error);
                _check_error(a.y, b.y, &max_error);
                _check_error(a.z, b.z, &max_error);
            }

            if (max_error > 0.0f); {
                printf("max error: %g\n\n", max_error);
                if (max_error > 0.01f)
                    assert(0);
            }
        }
    }
}

typedef enum {
    MC_UNIFORM,
    MC_UNIFORM_WITH_ONE_CLUMP,
} ModelCase;

static void _test(ModelCase model_case, TaySpaceType space_type, float see_radius, int depth_correction, Results *results, int steps) {
    srand(1);

    int dims = 3;
    int agents_count = 10000;
    float space_size = 1000.0f;
    float velocity = 1.0f;

    float4 see_radii = { see_radius, see_radius, see_radius, 0.0f };

    TayState *tay = tay_create_state(dims, see_radii);
    tay_set_source(tay, agent_kernels_source);

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

    int group = tay_add_group(tay, sizeof(Agent), agents_count);
    tay_add_see(tay, group, group, agent_see, "agent_see", see_radii, &see_context, sizeof(see_context));
    tay_add_act(tay, group, agent_act, "agent_act", &act_context, sizeof(act_context));

    switch (model_case) {
        case MC_UNIFORM: {
            _make_randomized_direction_cluster(tay,
                                               group,
                                               agents_count,
                                               float3_make(0.0f, 0.0f, 0.0f),
                                               float3_make(space_size, space_size, space_size),
                                               velocity);
        } break;
        case MC_UNIFORM_WITH_ONE_CLUMP: {
            int clump_count = agents_count / 2;
            _make_randomized_direction_cluster(tay,
                                               group,
                                               agents_count - clump_count,
                                               float3_make(0.0f, 0.0f, 0.0f),
                                               float3_make(space_size, space_size, space_size),
                                               velocity);
            _make_uniform_direction_cluster(tay,
                                            group,
                                            clump_count,
                                            float3_make(0.0f, 0.0f, 0.0f),
                                            float3_make(space_size * 0.1f, space_size * 0.1f, space_size * 0.1f),
                                            velocity);
        } break;
        default:
            assert(false); /* not implemented */
    }

    printf("R: %g, depth_correction: %d\n", see_radius, depth_correction);

    tay_simulation_start(tay);
    tay_run(tay, steps, space_type, depth_correction);
#if TAY_TELEMETRY
    tay_threads_report_telemetry(0);
#endif
    tay_simulation_end(tay);

    _write_or_compare_results(results, tay, group, agents_count);

    tay_destroy_state(tay);
}

void test() {

#if 1
    Results *r = _create_results();
#else
    Results *r = 0;
#endif

    int steps = 100;

    int beg_see_radius = 0;
    int end_see_radius = 1;

    int beg_depth_correction = 0;
    int end_depth_correction = 2;

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float perception_r = 50.0f * (1 << i);

#if 0
        printf("cpu simple:\n");
        _test(MC_UNIFORM, TAY_SPACE_CPU_SIMPLE, perception_r, 0, r, steps);
#endif

#if 1
        printf("cpu tree:\n");
        for (int j = beg_depth_correction; j < end_depth_correction; ++j)
            _test(MC_UNIFORM_WITH_ONE_CLUMP, TAY_SPACE_CPU_TREE, perception_r, j, r, steps);
#endif

#if 1
        printf("cpu grid:\n");
        for (int j = beg_depth_correction; j < end_depth_correction; ++j)
            _test(MC_UNIFORM_WITH_ONE_CLUMP, TAY_SPACE_CPU_GRID, perception_r, j, r, steps);
#endif

#if 0
        printf("gpu simple direct:\n");
        _test(MC_UNIFORM, TAY_SPACE_GPU_SIMPLE_DIRECT, perception_r, 0, r, steps);
#endif

#if 0
        printf("gpu simple indirect:\n");
        _test(MC_UNIFORM, TAY_SPACE_GPU_SIMPLE_INDIRECT, perception_r, 0, r, steps);
#endif

#if 0
        printf("gpu tree:\n");
        for (int j = beg_depth_correction; j < end_depth_correction; ++j)
            _test(MC_UNIFORM, TAY_SPACE_GPU_TREE, perception_r, j, r, steps);
#endif

#if 0
        printf("cycling:\n");
        for (int j = beg_depth_correction; j < end_depth_correction; ++j)
            _test(MC_UNIFORM, TAY_SPACE_CYCLE_ALL, perception_r, j, r, steps);
#endif

        printf("\n");

        _reset_results(r);
    }

    _destroy_results(r);
}
