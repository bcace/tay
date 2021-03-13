#include "test.h"
#include "thread.h"
#include "agent.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>


static double _test(ModelCase model_case, TaySpaceType space_type, float see_radius, int depth_correction, Results *results, int steps,
                    TayTelemetryResults *telemetry_results) {
    srand(1);

    int dims = 3;
    int agents_count = 10000;
    float space_size = 1000.0f;
    float velocity = 1.0f;

    float4 see_radii = { see_radius, see_radius, see_radius, 0.0f };

    TayState *tay = tay_create_state(1);
    tay_add_space(tay, space_type, dims, see_radii, depth_correction, 250);

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

    int group = tay_add_group(tay, sizeof(Agent), agents_count, 1, 0);
    tay_add_see(tay, group, group, agent_see, "agent_see", see_radii, &see_context, sizeof(see_context));
    tay_add_act(tay, group, agent_act, "agent_act", &act_context, sizeof(act_context));

    switch (model_case) {
        case MC_UNIFORM: {
            make_randomized_direction_cluster(tay,
                                               group,
                                               agents_count,
                                               float3_make(0.0f, 0.0f, 0.0f),
                                               float3_make(space_size, space_size, space_size),
                                               velocity);
        } break;
        case MC_UNIFORM_WITH_ONE_CLUMP: {
            int clump_count = (int)floor(agents_count * 0.2);
            make_randomized_direction_cluster(tay,
                                               group,
                                               agents_count - clump_count,
                                               float3_make(0.0f, 0.0f, 0.0f),
                                               float3_make(space_size, space_size, space_size),
                                               velocity);
            make_uniform_direction_cluster(tay,
                                            group,
                                            clump_count,
                                            float3_make(0.0f, 0.0f, 0.0f),
                                            float3_make(space_size * 0.05f, space_size * 0.05f, space_size * 0.05f),
                                            velocity);
        } break;
        default:
            printf("    model case not implemented\n");
    }

    printf("    depth_correction: %d\n", depth_correction);

    tay_simulation_start(tay);

    int steps_run = tay_run(tay, steps);
    if (steps_run == 0)
        printf("    run error: %d\n", tay_get_error(tay));

    double ms = tay_get_ms_per_step_for_last_run(tay);
    printf("    milliseconds per frame: %g\n", ms);

    tay_threads_get_telemetry_results(telemetry_results);
    tay_threads_report_telemetry(0);

    tay_simulation_end(tay);

    results_write_or_compare(results, tay, group, agents_count);

    tay_destroy_state(tay);

    return ms;
}

void test_basic(Results *results, int steps,
                int beg_see_radius, int end_see_radius,
                int beg_depth_correction, int end_depth_correction,
                ModelCase model_case, int space_type_flags) {

    FILE *plot;
    fopen_s(&plot, "plot.log", "w");

    fprintf(plot, "%d %d\n", beg_depth_correction, end_depth_correction);

    TayTelemetryResults telemetry_results;

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float see_radius = 50.0f * (1 << i);

        printf("see radius: %.2f\n", see_radius);

        if (space_type_flags & TAY_CPU_SIMPLE) {
            fprintf(plot, "CpuSimple::%d", i);
            printf("  cpu simple:\n");
            double ms = _test(model_case, TAY_CPU_SIMPLE, see_radius, 0, results, steps, &telemetry_results);
#if TAY_TELEMETRY
            fprintf(plot, " %g|%g|%g|%g|%g|%g\n",
                telemetry_results.mean_relative_deviation_averaged,
                telemetry_results.max_relative_deviation_averaged,
                telemetry_results.max_relative_deviation,
                telemetry_results.see_culling_efficiency,
                telemetry_results.mean_see_interactions_per_step,
                telemetry_results.grid_kernel_rebuilds);
#else
            fprintf(plot, " %g\n", ms);
#endif
        }

        if (space_type_flags & TAY_CPU_TREE) {
            fprintf(plot, "CpuTree::%d", i);
            printf("  cpu tree:\n");
            for (int j = beg_depth_correction; j < end_depth_correction; ++j) {
                double ms = _test(model_case, TAY_CPU_TREE, see_radius, j, results, steps, &telemetry_results);
#if TAY_TELEMETRY
                fprintf(plot, " %g|%g|%g|%g|%g|%g",
                    telemetry_results.mean_relative_deviation_averaged,
                    telemetry_results.max_relative_deviation_averaged,
                    telemetry_results.max_relative_deviation,
                    telemetry_results.see_culling_efficiency,
                    telemetry_results.mean_see_interactions_per_step,
                    telemetry_results.grid_kernel_rebuilds);
#else
                fprintf(plot, " %g", ms);
#endif
            }
            fprintf(plot, "\n");
        }

        if (space_type_flags & TAY_CPU_GRID) {
            fprintf(plot, "CpuGrid::%d", i);
            printf("  cpu grid:\n");
            for (int j = beg_depth_correction; j < end_depth_correction; ++j) {
                double ms = _test(model_case, TAY_CPU_GRID, see_radius, j, results, steps, &telemetry_results);
#if TAY_TELEMETRY
                fprintf(plot, " %g|%g|%g|%g|%g|%g",
                    telemetry_results.mean_relative_deviation_averaged,
                    telemetry_results.max_relative_deviation_averaged,
                    telemetry_results.max_relative_deviation,
                    telemetry_results.see_culling_efficiency,
                    telemetry_results.mean_see_interactions_per_step,
                    telemetry_results.grid_kernel_rebuilds);
#else
                fprintf(plot, " %g", ms);
#endif
            }
            fprintf(plot, "\n");
        }

        if (space_type_flags & TAY_GPU_SIMPLE_DIRECT) {
            fprintf(plot, "GpuSimple (direct)::%d", i);
            printf("  gpu simple direct:\n");
            double ms = _test(model_case, TAY_GPU_SIMPLE_DIRECT, see_radius, 0, results, steps, &telemetry_results);
#if TAY_TELEMETRY
            fprintf(plot, " %g|%g|%g|%g|%g|%g\n",
                telemetry_results.mean_relative_deviation_averaged,
                telemetry_results.max_relative_deviation_averaged,
                telemetry_results.max_relative_deviation,
                telemetry_results.see_culling_efficiency,
                telemetry_results.mean_see_interactions_per_step,
                telemetry_results.grid_kernel_rebuilds);
#else
            fprintf(plot, " %g\n", ms);
#endif
        }

        if (space_type_flags & TAY_GPU_SIMPLE_INDIRECT) {
            fprintf(plot, "GpuSimple (indirect)::%d", i);
            printf("  gpu simple indirect:\n");
            double ms = _test(model_case, TAY_GPU_SIMPLE_INDIRECT, see_radius, 0, results, steps, &telemetry_results);
#if TAY_TELEMETRY
            fprintf(plot, " %g|%g|%g|%g|%g|%g\n",
                telemetry_results.mean_relative_deviation_averaged,
                telemetry_results.max_relative_deviation_averaged,
                telemetry_results.max_relative_deviation,
                telemetry_results.see_culling_efficiency,
                telemetry_results.mean_see_interactions_per_step,
                telemetry_results.grid_kernel_rebuilds);
#else
            fprintf(plot, " %g\n", ms);
#endif
        }

        results_reset(results);
    }

    fclose(plot);
}
