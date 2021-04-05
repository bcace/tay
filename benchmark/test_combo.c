#include "test.h"
#include "agent.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>


static void _test(TaySpaceType space_type_a, TaySpaceType space_type_b, int steps, float see_radius, int part_res_a, int part_res_b, Results *results, FILE *file) {
    srand(1);

    float4 see_radii = { see_radius, see_radius, see_radius, see_radius };
    float4 part_radii_a = depth_correct(see_radii, part_res_a);
    float4 part_radii_b = depth_correct(see_radii, part_res_b);

    ActContext act_context;
    act_context.min.x = 0.0f;
    act_context.min.y = 0.0f;
    act_context.min.z = 0.0f;
    act_context.max.x = SPACE_SIZE;
    act_context.max.y = SPACE_SIZE;
    act_context.max.z = SPACE_SIZE;

    tay_log(file, "      {\n");
    tay_log(file, "        \"part radii a\": (%g, %g, %g),\n", part_radii_a.x, part_radii_a.y, part_radii_a.z);
    tay_log(file, "        \"part radii b\": (%g, %g, %g),\n", part_radii_b.x, part_radii_b.y, part_radii_b.z);

    TayState *tay = tay_create_state();
    TayGroup *group_a = tay_add_group(tay, sizeof(Agent), AGENTS_COUNT / 2, TAY_TRUE, tay_space_desc(space_type_a, 3, part_radii_a, 250));
    TayGroup *group_b = tay_add_group(tay, sizeof(Agent), AGENTS_COUNT / 2, TAY_TRUE, tay_space_desc(space_type_b, 3, part_radii_b, 250));

    make_randomized_direction_cluster(tay,
                                      group_a,
                                      AGENTS_COUNT / 2,
                                      float3_make(0.0f, 0.0f, 0.0f),
                                      float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE));

    make_randomized_direction_cluster(tay,
                                      group_b,
                                      AGENTS_COUNT / 2,
                                      float3_make(0.0f, 0.0f, 0.0f),
                                      float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE));

    tay_add_see(tay, group_a, group_b, agent_see, "agent_see", see_radii, 0, 0);
    tay_add_see(tay, group_a, group_a, agent_see, "agent_see", see_radii, 0, 0);
    tay_add_see(tay, group_b, group_a, agent_see, "agent_see", see_radii, 0, 0);
    tay_add_see(tay, group_b, group_b, agent_see, "agent_see", see_radii, 0, 0);
    tay_add_act(tay, group_a, agent_act, "agent_act", &act_context, sizeof(ActContext));
    tay_add_act(tay, group_b, agent_act, "agent_act", &act_context, sizeof(ActContext));

    tay_simulation_start(tay);
    int steps_run = tay_run(tay, steps);
    if (steps_run == 0)
        fprintf(stderr, "error %d\n", tay_get_error(tay));
    tay_log(file, "        \"ms per step\": %g,\n", tay_get_ms_per_step_for_last_run(tay));
    tay_threads_report_telemetry(0, file);
    results_write_or_compare(results, tay, group_a, AGENTS_COUNT / 2, offsetof(Agent, f_buffer), file);
    tay_log(file, "      },\n");
    tay_simulation_end(tay);

    tay_destroy_state(tay);
}

void test_combo(Results *results, int steps,
                 int beg_see_radius, int end_see_radius,
                 int beg_depth_correction, int end_depth_correction,
                 TaySpaceType *space_type_pairs) {

    FILE *file;
    #if TAY_TELEMETRY
    fopen_s(&file, "test_combo_telemetry.py", "w");
    #else
    fopen_s(&file, "test_combo_runtimes.py", "w");
    #endif

    tay_log(file, "data = {\n");

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float see_radius = SMALLEST_SEE_RADIUS * (1 << i);

        tay_log(file, "  %g: {\n", see_radius);

        for (int j = 0; space_type_pairs[j * 2] != TAY_SPACE_NONE; ++j) {
            TaySpaceType space_type_a = space_type_pairs[j * 2];
            TaySpaceType space_type_b = space_type_pairs[j * 2 + 1];

            tay_log(file, "    \"%s-%s\": [\n", space_type_name(space_type_a), space_type_name(space_type_b));
            _test(space_type_a, space_type_b, steps, see_radius, 0, 0, results, file);
            tay_log(file, "    ],\n");
        }

        results_reset(results);
        tay_log(file, "  },\n");
    }

    tay_log(file, "}\n");
    fclose(file);
}

void test_combo_nonpoint(Results *results, int steps,
                         int beg_see_radius, int end_see_radius,
                         int beg_depth_correction, int end_depth_correction,
                         TaySpaceType *space_type_pairs) {

}
