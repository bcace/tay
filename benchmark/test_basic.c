#include "test.h"
#include "thread.h"
#include "agent_host.h"
#include "taystd.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>


static void _test(ModelCase model_case, TaySpaceType space_type, float see_radius, int depth_correction, Results *results, int steps, FILE *file) {
    srand(1);

    float4 see_radii = { see_radius, see_radius, see_radius, 0.0f };
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

    SeeContext see_context;
    see_context.radii.x = see_radius;
    see_context.radii.y = see_radius;
    see_context.radii.z = see_radius;

    TayState *tay = tay_create_state();
    TayGroup *group = tay_add_group(tay, sizeof(Agent), AGENTS_COUNT, TAY_TRUE);
    tay_configure_space(tay, group, space_type, 3, part_sizes, 250);

    tay_add_see(tay, group, group, agent_see, "agent_see", see_radii, TAY_FALSE, &see_context, sizeof(see_context));
    tay_add_act(tay, group, agent_act, "agent_act", &act_context, sizeof(act_context));

    switch (model_case) {
        case MC_UNIFORM: {
            make_randomized_direction_cluster(tay,
                                              group,
                                              AGENTS_COUNT,
                                              float3_make(0.0f, 0.0f, 0.0f),
                                              float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE));
        } break;
        case MC_UNIFORM_WITH_ONE_CLUMP: {
            int clump_count = (int)floor(AGENTS_COUNT * 0.2);
            make_randomized_direction_cluster(tay,
                                              group,
                                              AGENTS_COUNT - clump_count,
                                              float3_make(0.0f, 0.0f, 0.0f),
                                              float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE));
            make_uniform_direction_cluster(tay,
                                           group,
                                           clump_count,
                                           float3_make(0.0f, 0.0f, 0.0f),
                                           float3_make(SPACE_SIZE * 0.05f, SPACE_SIZE * 0.05f, SPACE_SIZE * 0.05f));
        } break;
        default:
            fprintf(stderr, "model case not implemented\n");
    }

    ocl_add_source(tay, "agent.h");
    ocl_add_source(tay, "taystd.h");
    ocl_add_source(tay, "agent.c");
    ocl_add_source(tay, "taystd.c");

    tay_simulation_start(tay);
    int steps_run = tay_run(tay, steps);
    if (steps_run == 0)
        fprintf(stderr, "error %d\n", tay_get_error(tay));
    tay_log(file, "        \"ms per step\": %g,\n", tay_get_ms_per_step_for_last_run(tay));
    tay_threads_report_telemetry(0, file);
    results_write_or_compare(results, tay, group, AGENTS_COUNT, offsetof(Agent, f_buffer), offsetof(Agent, result_index), file);
    tay_log(file, "      },\n");
    tay_simulation_end(tay);

    tay_destroy_state(tay);
}

void test_basic(Results *results, ModelCase model_case, int steps,
                int beg_see_radius, int end_see_radius,
                int beg_depth_correction, int end_depth_correction,
                int space_type_flags) {

    FILE *file;
    #if TAY_TELEMETRY
    fopen_s(&file, "test_basic_telemetry.py", "w");
    #else
    fopen_s(&file, "test_basic_runtimes.py", "w");
    #endif

    tay_log(file, "data = {\n");

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float see_radius = SMALLEST_SEE_RADIUS * (1 << i);

        tay_log(file, "  %g: {\n", see_radius);

        if (space_type_flags & TAY_CPU_SIMPLE) {
            tay_log(file, "    \"%s\": [\n", space_type_name(TAY_CPU_SIMPLE));
            _test(model_case, TAY_CPU_SIMPLE, see_radius, 0, results, steps, file);
            tay_log(file, "    ],\n");
        }

        if (space_type_flags & TAY_CPU_KD_TREE) {
            tay_log(file, "    \"%s\": [\n", space_type_name(TAY_CPU_KD_TREE));
            for (int j = beg_depth_correction; j < end_depth_correction; ++j)
                _test(model_case, TAY_CPU_KD_TREE, see_radius, j, results, steps, file);
            tay_log(file, "    ],\n");
        }

        if (space_type_flags & TAY_CPU_GRID) {
            tay_log(file, "    \"%s\": [\n", space_type_name(TAY_CPU_GRID));
            for (int j = beg_depth_correction; j < end_depth_correction; ++j)
                _test(model_case, TAY_CPU_GRID, see_radius, j, results, steps, file);
            tay_log(file, "    ],\n");
        }

        if (space_type_flags & TAY_CPU_Z_GRID) {
            tay_log(file, "    \"%s\": [\n", space_type_name(TAY_CPU_Z_GRID));
            for (int j = beg_depth_correction; j < end_depth_correction; ++j)
                _test(model_case, TAY_CPU_Z_GRID, see_radius, j, results, steps, file);
            tay_log(file, "    ],\n");
        }

        if (space_type_flags & TAY_OCL_SIMPLE) {
            tay_log(file, "    \"%s\": [\n", space_type_name(TAY_OCL_SIMPLE));
            _test(model_case, TAY_OCL_SIMPLE, see_radius, 0, results, steps, file);
            tay_log(file, "    ],\n");
        }

        results_reset(results);
        tay_log(file, "  },\n");
    }

    tay_log(file, "}\n");
    fclose(file);
}
