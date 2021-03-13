#include "test.h"
#include "agent.h"
#include <stdlib.h>
#include <stdio.h>


Results *_create_results() {
    Results *r = malloc(sizeof(Results));
    r->first_time = 1;
    return r;
}

void _reset_results(Results *r) {
    if (r)
        r->first_time = 1;
}

void _destroy_results(Results *r) {
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

void _write_or_compare_results(Results *results, TayState *tay, int group, int agents_count) {
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
