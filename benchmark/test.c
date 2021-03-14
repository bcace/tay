#include "test.h"
#include "agent.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


float4 depth_correct(float4 radii, unsigned level) {
    float c = (float)(1 << level);
    return (float4) { radii.x / c, radii.y / c, radii.z / c, radii.w / c };
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

void results_write_or_compare(Results *results, TayState *tay, int group, int agents_count) {
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
                printf("    max error: %g\n", max_error);
                if (max_error > 0.01f)
                    printf("    !!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            }
        }
    }
}

void make_randomized_direction_cluster(TayState *state, int group, int count, float3 min, float3 max, float velocity) {
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

void make_uniform_direction_cluster(TayState *state, int group, int count, float3 min, float3 max, float velocity) {
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

