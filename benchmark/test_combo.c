#include "test.h"
#include "state.h"
#include "agent_host.h"
#include "taystd.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>


static TAY_SEE_FUNC _get_see_func(int is_point_a, int is_point_b) {
    if (is_point_a && is_point_b)
        return agent_see;
    else if (!is_point_a && !is_point_b)
        return box_agent_see;
    return 0;
}

static char *_get_see_func_name(int is_point_a, int is_point_b) {
    if (is_point_a && is_point_b)
        return "agent_see";
    else if (!is_point_a && !is_point_b)
        return "box_agent_see";
    return 0;
}

static TAY_ACT_FUNC _get_act_func(int is_point) {
    if (is_point)
        return agent_act;
    else
        return box_agent_act;
}

static char *_get_act_func_name(int is_point) {
    if (is_point)
        return "agent_act";
    else
        return "box_agent_act";
}

static void _test(Config *config, int is_point_a, int is_point_b, int steps, float see_radius, int part_res_a, int part_res_b, Results *results, FILE *file) {
    srand(1);

    float4 see_radii = { see_radius, see_radius, see_radius, see_radius };
    float4 part_sizes_a = interaction_radii_to_partition_sizes(see_radii, part_res_a);
    float4 part_sizes_b = interaction_radii_to_partition_sizes(see_radii, part_res_b);

    ActContext act_context;
    act_context.min.x = 0.0f;
    act_context.min.y = 0.0f;
    act_context.min.z = 0.0f;
    act_context.max.x = SPACE_SIZE;
    act_context.max.y = SPACE_SIZE;
    act_context.max.z = SPACE_SIZE;

    tay_log(file, "      {\n");
    tay_log(file, "        \"part sizes a\": (%g, %g, %g),\n", part_sizes_a.x, part_sizes_a.y, part_sizes_a.z);
    tay_log(file, "        \"part sizes b\": (%g, %g, %g),\n", part_sizes_b.x, part_sizes_b.y, part_sizes_b.z);

    float min_size = 1.0f;
    float max_size = 50.0f;
    float distr_exp = 0.0f;

    TayState *tay = tay_create_state();
    tay_state_enable_ocl(tay);
    TayGroup *group_a;
    TayGroup *group_b;

    if (is_point_a) {
        group_a = tay_add_group(tay, sizeof(Agent), AGENTS_COUNT / 2, is_point_a);
        tay_configure_space(tay, group_a, config->a_type, 3, part_sizes_a, 250);

        make_randomized_direction_cluster(tay, group_a, AGENTS_COUNT / 2,
                                          float3_make(0.0f, 0.0f, 0.0f),
                                          float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE));
    }
    else {
        group_a = tay_add_group(tay, sizeof(BoxAgent), AGENTS_COUNT / 2, is_point_a);
        tay_configure_space(tay, group_a, config->b_type, 3, part_sizes_a, 250);

        make_randomized_direction_cluster_nonpoint(tay, group_a, AGENTS_COUNT / 2,
                                                   float3_make(0.0f, 0.0f, 0.0f),
                                                   float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE),
                                                   min_size, max_size, distr_exp);
    }
    if (config->a_ocl_enabled)
        tay_group_enable_ocl(tay, group_a);

    if (is_point_b) {
        group_b = tay_add_group(tay, sizeof(Agent), AGENTS_COUNT / 2, is_point_b);
        tay_configure_space(tay, group_b, config->b_type, 3, part_sizes_b, 250);

        make_randomized_direction_cluster(tay,
                                          group_b,
                                          AGENTS_COUNT / 2,
                                          float3_make(0.0f, 0.0f, 0.0f),
                                          float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE));
    }
    else {
        group_b = tay_add_group(tay, sizeof(BoxAgent), AGENTS_COUNT / 2, is_point_b);
        tay_configure_space(tay, group_b, config->b_type, 3, part_sizes_b, 250);

        make_randomized_direction_cluster_nonpoint(tay, group_b, AGENTS_COUNT / 2,
                                                   float3_make(0.0f, 0.0f, 0.0f),
                                                   float3_make(SPACE_SIZE, SPACE_SIZE, SPACE_SIZE),
                                                   min_size, max_size, distr_exp);
    }
    if (config->b_ocl_enabled)
        tay_group_enable_ocl(tay, group_b);

    tay_add_see(tay, group_a, group_a, _get_see_func(is_point_a, is_point_b), _get_see_func_name(is_point_a, is_point_b), see_radii, TAY_FALSE, 0, 0);
    tay_add_see(tay, group_a, group_b, _get_see_func(is_point_a, is_point_b), _get_see_func_name(is_point_a, is_point_b), see_radii, TAY_FALSE, 0, 0);
    tay_add_see(tay, group_b, group_b, _get_see_func(is_point_a, is_point_b), _get_see_func_name(is_point_a, is_point_b), see_radii, TAY_FALSE, 0, 0);
    tay_add_see(tay, group_b, group_a, _get_see_func(is_point_a, is_point_b), _get_see_func_name(is_point_a, is_point_b), see_radii, TAY_FALSE, 0, 0);
    tay_add_act(tay, group_a, _get_act_func(is_point_a), _get_act_func_name(is_point_a), &act_context, sizeof(act_context));
    tay_add_act(tay, group_b, _get_act_func(is_point_b), _get_act_func_name(is_point_b), &act_context, sizeof(act_context));

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
    int f_buffer_offset;
    int result_index_offset;
    if (is_point_a) {
        f_buffer_offset = offsetof(Agent, f_buffer);
        result_index_offset = offsetof(Agent, result_index);
    }
    else {
        f_buffer_offset = offsetof(BoxAgent, f_buffer);
        result_index_offset = offsetof(BoxAgent, result_index);
    }
    results_write_or_compare(results, tay, group_a, AGENTS_COUNT / 2, f_buffer_offset, result_index_offset, file);
    tay_log(file, "      },\n");
    tay_simulation_end(tay);

    tay_destroy_state(tay);
}

void test_combo(Results *results, int steps, int is_point_a, int is_point_b,
                int beg_see_radius, int end_see_radius,
                int beg_depth_correction, int end_depth_correction,
                Configs *configs) {

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

        for (unsigned config_i = 0; config_i < configs->count; ++config_i) {
            Config *config = configs->configs + config_i;

            tay_log(file, "    \"%s\": [\n", space_label(config));

            if (space_can_depth_correct(config))
                for (int dc_i = beg_depth_correction; dc_i < end_depth_correction; ++dc_i)
                    _test(config, is_point_a, is_point_b, steps, see_radius, dc_i, dc_i, results, file);
            else
                _test(config, is_point_a, is_point_b, steps, see_radius, 0, 0, results, file);

            tay_log(file, "    ],\n");
        }

        results_reset(results);
        tay_log(file, "  },\n");
    }

    tay_log(file, "}\n");
    fclose(file);
}
