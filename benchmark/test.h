#ifndef test_h
#define test_h

#include "tay.h"

#define SMALLEST_SEE_RADIUS 50.0f

typedef enum {
    MC_UNIFORM,
    MC_UNIFORM_WITH_ONE_CLUMP,
} ModelCase;

typedef struct {
    float4 data[10000000];
    int first_time;
} Results;

Results *results_create();
void results_reset(Results *r);
void results_destroy(Results *r);
void results_write_or_compare(Results *results, TayState *tay, int group, int agents_count);

void make_randomized_direction_cluster(TayState *state, int group, int count, float3 min, float3 max, float velocity);
void make_uniform_direction_cluster(TayState *state, int group, int count, float3 min, float3 max, float velocity);

float4 depth_correct(float4 radii, unsigned level);

void test_basic(Results *results, int steps,
                int beg_see_radius, int end_see_radius,
                int beg_depth_correction, int end_depth_correction,
                ModelCase model_case, int space_type_flags);

void test_nonpoint(Results *results, int steps,
                   int beg_see_radius, int end_see_radius,
                   int beg_depth_correction, int end_depth_correction);

#endif
