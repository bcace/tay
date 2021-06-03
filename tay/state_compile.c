#include "state.h"


int state_compile(TayState *state) {

    /**/
    {
        unsigned groups_count = 0;
        unsigned ocl_groups_count = 0;

        for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            TayGroup *group = state->groups + group_i;

            if (group_is_inactive(group))
                continue;

            ++groups_count;
            if (group->ocl_enabled)
                ++ocl_groups_count;
        }

        if (ocl_groups_count && !state->ocl.enabled) {
            ocl_enable(state);

            if (!state->ocl.enabled) {
                tay_set_error2(state, TAY_ERROR_OCL, "found an OpenCL structure, but the OpenCL context is disabled");
                return 0;
            }
        }
    }

    return 1;
}
