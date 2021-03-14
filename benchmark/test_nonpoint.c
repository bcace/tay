#include "test.h"
#include "agent.h"
#include <stdlib.h>


void _test(TaySpaceType space_type, float see_radius, int depth_correction) {
    srand(1);

    int agents_count = 10000;
    float4 see_radii = { see_radius, see_radius, see_radius, see_radius };

    TayState *tay = tay_create_state();

    tay_add_space(tay, space_type, 3, depth_correct(see_radii, depth_correction), 250);

    ActContext act_context;
    act_context.min.x = 0.0f;
    act_context.min.y = 0.0f;
    act_context.min.z = 0.0f;
    act_context.max.x = SPACE_SIZE;
    act_context.max.y = SPACE_SIZE;
    act_context.max.z = SPACE_SIZE;

    int group = tay_add_group(tay, sizeof(BoxAgent), agents_count, TAY_FALSE, 0);
    // tay_add_see(tay, group, group, box_agent_see, "box_agent_see", see_radii, 0, 0);
    // tay_add_act(tay, group, box_agent_act, "box_agent_act", &act_context, sizeof(ActContext));

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
