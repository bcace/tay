#include "test.h"
#include "thread.h"
#include "agent_host.h"
#include "taystd.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


void _test(TaySpaceType space_type, int steps, float see_radius, int depth_correction, float min_size, float max_size, float distr_exp, Results *results, FILE *file) {
    srand(1);

    float4 see_radii = { see_radius, see_radius, see_radius, see_radius };
    float4 part_sizes = interaction_radii_to_partition_sizes(see_radii, depth_correction);

    tay_log(file, "      {\n");
    tay_log(file, "        \"part_sizes\": (%g, %g, %g),\n", part_sizes.x, part_sizes.y, part_sizes.z);

    ActContext act_context;
    act_context.min.x = 0.0f;
    act_context.min.y = 0.0f;
    act_context.min.z = 0.0f;
    act_context.max.x = SPACE_SIZE;
    act_context.max.y = SPACE_SIZE;
    act_context.max.z = SPACE_SIZE;

    TayState *tay = tay_create_state();
    TayGroup *group = tay_add_group(tay, sizeof(BoxAgent), AGENTS_COUNT, TAY_FALSE);
    tay_configure_space(tay, group, space_type, 3, part_sizes, 250);

    tay_add_see(tay, group, group, box_agent_see, see_radii, TAY_FALSE, 0);
    tay_add_act(tay, group, box_agent_act, &act_context);

    make_randomized_direction_cluster_nonpoint(tay, group, AGENTS_COUNT,
                                               float3_make(0, 0, 0),
                                               float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE),
                                               min_size, max_size, distr_exp);

    tay_simulation_start(tay);
    int steps_run = tay_run(tay, steps);
    if (steps_run == 0)
        fprintf(stderr, "error %d\n", tay_get_error(tay));
    tay_log(file, "        \"ms per step\": %g,\n", tay_get_ms_per_step_for_last_run(tay));
    tay_threads_report_telemetry(0, file);
    results_write_or_compare(results, tay, group, AGENTS_COUNT, offsetof(BoxAgent, f_buffer), offsetof(BoxAgent, result_index), file);
    tay_log(file, "      },\n");
    tay_simulation_end(tay);

    tay_destroy_state(tay);
}

void test_nonpoint(Results *results, int steps,
                   int beg_see_radius, int end_see_radius,
                   int beg_depth_correction, int end_depth_correction,
                   int space_type_flags) {

    float min_size = 1.0f;
    float max_size = 50.0f;
    float distr_exp = 0.0f;

    FILE *file;
    #if TAY_TELEMETRY
    fopen_s(&file, "test_nonpoint_telemetry.py", "w");
    #else
    fopen_s(&file, "test_nonpoint_runtimes.py", "w");
    #endif

    tay_log(file, "data = {\n");

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float see_radius = SMALLEST_SEE_RADIUS * (1 << i);

        tay_log(file, "  %g: {\n", see_radius);

        if (space_type_flags & TAY_CPU_SIMPLE) {
            tay_log(file, "    \"%s\": [\n", space_type_name(TAY_CPU_SIMPLE));
            _test(TAY_CPU_SIMPLE, steps, see_radius, 0, min_size, max_size, distr_exp, results, file);
            tay_log(file, "    ],\n");
        }

        if (space_type_flags & TAY_CPU_KD_TREE) {
            tay_log(file, "    \"%s\": [\n", space_type_name(TAY_CPU_KD_TREE));
            for (int depth_correction = beg_depth_correction; depth_correction < end_depth_correction; ++depth_correction)
                _test(TAY_CPU_KD_TREE, steps, see_radius, depth_correction, min_size, max_size, distr_exp, results, file);
            tay_log(file, "    ],\n");
        }

        if (space_type_flags & TAY_CPU_AABB_TREE) {
            tay_log(file, "    \"%s\": [\n", space_type_name(TAY_CPU_AABB_TREE));
            for (int depth_correction = beg_depth_correction; depth_correction < end_depth_correction; ++depth_correction)
                _test(TAY_CPU_AABB_TREE, steps, see_radius, depth_correction, min_size, max_size, distr_exp, results, file);
            tay_log(file, "    ],\n");
        }

        results_reset(results);
        tay_log(file, "  },\n");
    }

    tay_log(file, "}\n");
    fclose(file);
}
