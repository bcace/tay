#include "tay.h"
#include "test.h"
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

typedef enum {
    MC_UNIFORM,
    MC_UNIFORM_WITH_ONE_CLUMP,
} ModelCase;

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
            _make_randomized_direction_cluster(tay,
                                               group,
                                               agents_count,
                                               float3_make(0.0f, 0.0f, 0.0f),
                                               float3_make(space_size, space_size, space_size),
                                               velocity);
        } break;
        case MC_UNIFORM_WITH_ONE_CLUMP: {
            int clump_count = (int)floor(agents_count * 0.2);
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
                                            float3_make(space_size * 0.05f, space_size * 0.05f, space_size * 0.05f),
                                            velocity);
        } break;
        default:
            assert(false); /* not implemented */
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

    _write_or_compare_results(results, tay, group, agents_count);

    tay_destroy_state(tay);

    return ms;
}

int main() {
    tay_threads_start();

#if 1
    Results *results = _create_results();
#else
    Results *results = 0;
#endif

    int steps = 100;
    int model_case = MC_UNIFORM;

    int beg_see_radius = 0;
    int end_see_radius = 2;

    int beg_depth_correction = 0;
    int end_depth_correction = 3;

    bool run_cpu_simple = false;
    bool run_cpu_tree = false;
    bool run_cpu_grid = false;
    bool run_gpu_simple_direct = true;
    bool run_gpu_simple_indirect = false;

    FILE *plot;
    fopen_s(&plot, "plot", "w");

    fprintf(plot, "%d %d\n", beg_depth_correction, end_depth_correction);

    TayTelemetryResults telemetry_results;

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float see_radius = 50.0f * (1 << i);

        printf("see radius: %.2f\n", see_radius);

        if (run_cpu_simple) {
            fprintf(plot, "CpuSimple::%d", i);
            printf("  cpu simple:\n");
            double ms = _test(model_case, TAY_SPACE_CPU_SIMPLE, see_radius, 0, results, steps, &telemetry_results);
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

        if (run_cpu_tree) {
            fprintf(plot, "CpuTree::%d", i);
            printf("  cpu tree:\n");
            for (int j = beg_depth_correction; j < end_depth_correction; ++j) {
                double ms = _test(model_case, TAY_SPACE_CPU_TREE, see_radius, j, results, steps, &telemetry_results);
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

        if (run_cpu_grid) {
            fprintf(plot, "CpuGrid::%d", i);
            printf("  cpu grid:\n");
            for (int j = beg_depth_correction; j < end_depth_correction; ++j) {
                double ms = _test(model_case, TAY_SPACE_CPU_GRID, see_radius, j, results, steps, &telemetry_results);
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

        if (run_gpu_simple_direct) {
            fprintf(plot, "GpuSimple (direct)::%d", i);
            printf("  gpu simple direct:\n");
            double ms = _test(model_case, TAY_SPACE_GPU_SIMPLE_DIRECT, see_radius, 0, results, steps, &telemetry_results);
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

        if (run_gpu_simple_indirect) {
            fprintf(plot, "GpuSimple (indirect)::%d", i);
            printf("  gpu simple indirect:\n");
            double ms = _test(model_case, TAY_SPACE_GPU_SIMPLE_INDIRECT, see_radius, 0, results, steps, &telemetry_results);
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

        _reset_results(results);
    }

    _destroy_results(results);

    fclose(plot);

    tay_threads_stop();
    return 0;
}
