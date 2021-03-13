#include "test.h"
#include <stdlib.h>


void _test(TaySpaceType space_type, float see_radius, int depth_correction) {
    srand(1);

    float4 see_radii = { see_radius, see_radius, see_radius, see_radius };

    TayState *tay = tay_create_state();

    tay_add_space(tay, space_type, 3, depth_correct(see_radii, depth_correction), 250);

    // int group = tay_add_group(tay, sizeof(BoxAgent), agents_count, 1, 0);

    tay_destroy_state(tay);
}

void test_nonpoint(Results *results, int steps,
                   int beg_see_radius, int end_see_radius,
                   int beg_depth_correction, int end_depth_correction) {

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float see_radius = SMALLEST_SEE_RADIUS * (1 << i);

        _test(TAY_CPU_SIMPLE, see_radius, 0);
    }
}
