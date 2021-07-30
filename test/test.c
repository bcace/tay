#include "test.h"
#include "agent_host.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>


/* Returns suggested partition sizes calculated from interaction radii.
Positive "level" makes partitions smaller, negative makes them larger. */
float4 interaction_radii_to_partition_sizes(float4 radii, int level) {
    float c = (1.0f - level * 0.1f) * 2.0f;
    return (float4){ radii.x * c, radii.y * c, radii.z * c, radii.w * c };
}

void spaces_init(Configs *configs) {
    configs->count = 0;
}

void space_add_single(Configs *configs, TaySpaceType a_type, int is_ocl) {
    Config *config = configs->configs + configs->count++;
    config->a_type = a_type;
    config->b_type = TAY_SPACE_NONE;
    config->is_ocl = is_ocl;
}

void space_add_double(Configs *configs, TaySpaceType a_type, TaySpaceType b_type, int is_ocl) {
    Config *config = configs->configs + configs->count++;
    config->a_type = a_type;
    config->b_type = b_type;
    config->is_ocl = is_ocl;
}

int space_can_depth_correct(Config *config) {
    if (config->b_type == TAY_SPACE_NONE)
        return config->a_type != TAY_CPU_GRID;
    else
        return config->a_type != TAY_CPU_GRID || config->b_type != TAY_CPU_GRID;
}

const char *_space_type_name(TaySpaceType space_type) {
    if (space_type == TAY_SPACE_NONE)
        return "(None)";
    else {
        switch (space_type) {
            case TAY_CPU_SIMPLE: return "Simple";
            case TAY_CPU_KD_TREE: return "Kd Tree";
            case TAY_CPU_AABB_TREE: return "AABB Tree";
            case TAY_CPU_GRID: return "Grid";
            case TAY_CPU_Z_GRID: return "Z Grid";
            default: return "(None)";
        }
    }
}

char *space_label(Config *config) {
    static char buffer[256];
    buffer[0] = '\0';

    if (config->b_type == TAY_SPACE_NONE)
        sprintf_s(buffer, 256, "%s (%s)", _space_type_name(config->a_type), config->is_ocl ? "GPU" : "CPU");
    else
        sprintf_s(buffer, 256, "%s - %s (%s)", _space_type_name(config->a_type), _space_type_name(config->b_type), config->is_ocl ? "GPU" : "CPU");

    return buffer;
}

Results *results_create() {
    Results *r = malloc(sizeof(Results));
    r->first_time = 1;
    return r;
}

void results_reset(Results *r) {
    if (r)
        r->first_time = 1;
}

void results_destroy(Results *r) {
    if (r)
        free(r);
}

static inline void _check_error(float a, float b, float *max_error) {
    float relative_error = (a - b) / a;
    if (relative_error > *max_error)
        *max_error = relative_error;
    else if (-relative_error > *max_error)
        *max_error = -relative_error;
}

void results_write_or_compare(Results *results, TayState *tay, TayGroup *group, int agents_count, int f_buffer_offset, int result_index_offset, void *file) {
    if (results) {
        if (results->first_time) {
            for (int i = 0; i < agents_count; ++i) {
                char *agent = tay_get_agent(tay, group, i);
                int result_index = *(int *)(agent + result_index_offset);
                results->data[result_index] = *(float4 *)(agent + f_buffer_offset);
            }
            results->first_time = 0;
        }
        else {
            float max_error = 0.0f;

            for (int i = 0; i < agents_count; ++i) {
                char *agent = tay_get_agent(tay, group, i);
                int result_index = *(int *)(agent + result_index_offset);
                float4 a = results->data[result_index];
                float4 b = *(float4 *)(agent + f_buffer_offset);
                _check_error(a.x, b.x, &max_error);
                _check_error(a.y, b.y, &max_error);
                _check_error(a.z, b.z, &max_error);
            }

            if (max_error > 0.0f); {
                tay_log(file, "        \"max error\": %g\n", max_error);
                if (max_error > 0.01f)
                    fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
            }
        }
    }
}

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

static float _rand_exponential(float min, float max, float exp) {
    float base = rand() / (float)RAND_MAX;
    return min + (max - min) * powf(base, exp);
}

void make_randomized_direction_cluster_nonpoint(TayState *state, TayGroup *group, int count, float3 min, float3 max, float min_size, float max_size, float distr_exp) {
    for (int i = 0; i < count; ++i) {
        BoxAgent *a = tay_get_available_agent(state, group);

        /* size */
        float size = _rand_exponential(min_size, max_size, distr_exp);

        /* position */
        a->min.x = _rand(min.x, max.x - size);
        a->min.y = _rand(min.y, max.y - size);
        a->min.z = _rand(min.z, max.z - size);
        a->max.x = a->min.x + size;
        a->max.y = a->min.y + size;
        a->max.z = a->min.z + size;

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

        a->result_index = i;

        tay_commit_available_agent(state, group);
    }
}

void make_randomized_direction_cluster(TayState *state, TayGroup *group, int count, float3 min, float3 max) {
    for (int i = 0; i < count; ++i) {
        Agent *a = tay_get_available_agent(state, group);

        /* position */
        a->p.x = _rand(min.x, max.x);
        a->p.y = _rand(min.y, max.y);
        a->p.z = _rand(min.z, max.z);

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

        a->result_index = i;

        tay_commit_available_agent(state, group);
    }
}

void make_uniform_direction_cluster(TayState *state, TayGroup *group, int count, float3 min, float3 max) {
    float3 v;

    /* uniform velocity */
    v.x = _rand(-1.0f, 1.0f);
    v.y = _rand(-1.0f, 1.0f);
    v.z = _rand(-1.0f, 1.0f);
    float l = AGENT_VELOCITY / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    v.x *= l;
    v.y *= l;
    v.z *= l;

    for (int i = 0; i < count; ++i) {
        Agent *a = tay_get_available_agent(state, group);

        /* position */
        a->p.x = _rand(min.x, max.x);
        a->p.y = _rand(min.y, max.y);
        a->p.z = _rand(min.z, max.z);

        /* velocity */
        a->v.x = v.x;
        a->v.y = v.y;
        a->v.z = v.z;

        /* buffers */
        a->b_buffer.x = 0.0f;
        a->b_buffer.y = 0.0f;
        a->b_buffer.z = 0.0f;
        a->b_buffer_count = 0;
        a->f_buffer.x = 0.0f;
        a->f_buffer.y = 0.0f;
        a->f_buffer.z = 0.0f;

        a->result_index = i;

        tay_commit_available_agent(state, group);
    }
}
