#ifndef test_h
#define test_h

#include "tay.h"


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

void test_basic(Results *results);

#endif
