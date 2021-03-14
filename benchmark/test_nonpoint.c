#include "test.h"
#include "thread.h"
#include "agent.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

static float _rand_exponential(float min, float max, float exp) {
    float base = rand() / (float)RAND_MAX;
    return min + (max - min) * powf(base, exp);
}

static void _make_randomized_direction_cluster(TayState *state, int group, int count, float3 min, float3 max, float min_size, float max_size) {
    for (int i = 0; i < count; ++i) {
        int major = i % 3;

        /* size */
        float size = _rand_exponential(min_size, max_size, 1);

        /* shape */
        float3 shape;
        for (int j = 0; j < 3; ++j) {
            if (j == major)
                shape.arr[j] = size;
            else
                shape.arr[j] = size * _rand(0.1f, 1.0f);
        }

        /* position */
        BoxAgent *a = tay_get_available_agent(state, group);
        a->min.x = _rand(min.x, max.x - shape.x);
        a->min.y = _rand(min.y, max.y - shape.y);
        a->min.z = _rand(min.z, max.z - shape.z);
        a->max.x = a->min.x + shape.x;
        a->max.y = a->min.y + shape.y;
        a->max.z = a->min.z + shape.z;

        /* velocity */
        a->v.x = _rand(-1.0f, 1.0f);
        a->v.y = _rand(-1.0f, 1.0f);
        a->v.z = _rand(-1.0f, 1.0f);
        float l = AGENT_VELOCITY / sqrtf(a->v.x * a->v.x + a->v.y * a->v.y + a->v.z * a->v.z);
        a->v.x *= l;
        a->v.y *= l;
        a->v.z *= l;

        /* buffers */
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

void _test(TaySpaceType space_type, int steps, float see_radius, int depth_correction, float min_size, float max_size, Results *results) {
    srand(1);

    printf("  %s (%d):\n", space_type_name(space_type), depth_correction);

    int agents_count = 10000;
    float4 see_radii = { see_radius, see_radius, see_radius, see_radius };

    ActContext act_context;
    act_context.min.x = 0.0f;
    act_context.min.y = 0.0f;
    act_context.min.z = 0.0f;
    act_context.max.x = SPACE_SIZE;
    act_context.max.y = SPACE_SIZE;
    act_context.max.z = SPACE_SIZE;

    TayState *tay = tay_create_state();
    tay_add_space(tay, space_type, 3, depth_correct(see_radii, depth_correction), 250);

    int group = tay_add_group(tay, sizeof(BoxAgent), agents_count, TAY_FALSE, 0);
    tay_add_see(tay, group, group, box_agent_see, "box_agent_see", see_radii, 0, 0);
    tay_add_act(tay, group, box_agent_act, "box_agent_act", &act_context, sizeof(ActContext));

    _make_randomized_direction_cluster(tay, group, agents_count,
                                       float3_make(0, 0, 0),
                                       float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE),
                                       min_size, max_size);

    tay_simulation_start(tay);

    int steps_run = tay_run(tay, steps);
    if (steps_run == 0)
        printf("    error %d", tay_get_error(tay));

    double ms = tay_get_ms_per_step_for_last_run(tay);
    printf("    %g ms\n", ms);

    results_write_or_compare(results, tay, group, agents_count, offsetof(BoxAgent, f_buffer));

    tay_threads_report_telemetry(0);
    tay_simulation_end(tay);
    tay_destroy_state(tay);
}

void test_nonpoint(Results *results, int steps,
                   int beg_see_radius, int end_see_radius,
                   int beg_depth_correction, int end_depth_correction,
                   int space_type_flags) {

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float see_radius = SMALLEST_SEE_RADIUS * (1 << i);

        printf("R: %g\n", see_radius);

        float min_size = 10.0f;
        float max_size = 200.0f;

        if (space_type_flags & TAY_CPU_SIMPLE)
            _test(TAY_CPU_SIMPLE, steps, see_radius, 0, min_size, max_size, results);

        if (space_type_flags & TAY_CPU_TREE)
            for (int depth_correction = beg_depth_correction; depth_correction < end_depth_correction; ++depth_correction)
                _test(TAY_CPU_TREE, steps, see_radius, depth_correction, min_size, max_size, results);

        results_reset(results);
    }
}
