#ifndef test_h
#define test_h

#include "tay.h"


typedef struct {
    float4 data[10000000];
    int first_time;
} Results;

Results *_create_results();
void _reset_results(Results *r);
void _destroy_results(Results *r);
void _write_or_compare_results(Results *results, TayState *tay, int group, int agents_count);

#endif
