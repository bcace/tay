#ifndef test_h
#define test_h

#include "tay.h"

#define SMALLEST_SEE_RADIUS     50.0f
#define SPACE_SIZE              1000.0f
#define AGENT_VELOCITY          1.0f
#define AGENTS_COUNT            10000


typedef enum {
    MC_UNIFORM,
    MC_UNIFORM_WITH_ONE_CLUMP,
} ModelCase;

typedef struct {
    float4 data[1000000];
    int first_time;
} Results;

Results *results_create();
void results_reset(Results *r);
void results_destroy(Results *r);
void results_write_or_compare(Results *results, TayState *tay, TayGroup *group, int agents_count, int f_buffer_offset, int result_index_offset, void *file);

void make_randomized_direction_cluster(TayState *state, TayGroup *group, int count, float3 min, float3 max);
void make_uniform_direction_cluster(TayState *state, TayGroup *group, int count, float3 min, float3 max);
void make_randomized_direction_cluster_nonpoint(TayState *state, TayGroup *group, int count, float3 min, float3 max, float min_size, float max_size, float distr_exp);

float4 depth_correct(float4 radii, int level);
const char *space_type_name(TaySpaceType space_type);

void test_basic(Results *results, ModelCase model_case, int steps,
                int beg_see_radius, int end_see_radius,
                int beg_depth_correction, int end_depth_correction,
                int space_type_flags);

void test_nonpoint(Results *results, int steps,
                   int beg_see_radius, int end_see_radius,
                   int beg_depth_correction, int end_depth_correction,
                   int space_type_flags);

void test_combo(Results *results, int steps, int is_point_a, int is_point_b,
                int beg_see_radius, int end_see_radius,
                int beg_depth_correction, int end_depth_correction,
                TaySpaceType *spec_pairs);

#endif
