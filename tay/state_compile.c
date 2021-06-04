#include "state.h"
#include "space.h"


int state_compile(TayState *state) {
    unsigned groups_count = 0;
    unsigned ocl_groups_count = 0;

    /* check OCL state */
    {
        for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
            TayGroup *group = state->groups + group_i;

            if (group_is_inactive(group))
                continue;

            ++groups_count;
            if (group->ocl_enabled)
                ++ocl_groups_count;
        }

        if (ocl_groups_count && !state->ocl.device.enabled) {
            ocl_enable(state);

            if (!state->ocl.device.enabled) {
                tay_set_error2(state, TAY_ERROR_OCL, "found an OCL structure, but the OCL context is disabled");
                return 0;
            }
        }
    }

    /* compose and compile OCL program */
    if (ocl_groups_count) {
        if (!ocl_compile_program(state))
            return 0;
    }

    /* check passes */
    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;
            int seer_is_point = pass->seer_group->is_point;
            int seen_is_point = pass->seen_group->is_point;
            int seer_is_ocl = pass->seer_group->ocl_enabled;
            int seen_is_ocl = pass->seen_group->ocl_enabled;

            if (seer_space->dims != seen_space->dims) {
                tay_set_error2(state, TAY_ERROR_NOT_IMPLEMENTED, "groups with different numbers of dimensions not supported");
                return 0;
            }

            if (seer_is_ocl && seen_is_ocl) { /* both groups must be ocl enabled to have an ocl pass */
                if (seer_space->type != TAY_CPU_SIMPLE && seer_space->type != TAY_CPU_GRID) {
                    tay_set_error2(state, TAY_ERROR_OCL, "unhandled OCL see pass seer type");
                    return 0;
                }

                if (seen_space->type != TAY_CPU_SIMPLE && seen_space->type != TAY_CPU_GRID) {
                    tay_set_error2(state, TAY_ERROR_OCL, "unhandled OCL see pass seen type");
                    return 0;
                }

                if (!ocl_get_pass_kernel(state, pass))
                    return 0;
            }
            else {
                if (pass->seer_group == pass->seen_group) {
                    if (pass->self_see)
                        pass->pairing_func = (seer_is_point) ? space_see_point_point_self_see : space_see_nonpoint_nonpoint_self_see;
                    else
                        pass->pairing_func = (seer_is_point) ? space_see_point_point : space_see_nonpoint_nonpoint;
                }
                else {
                    if (seer_is_point == seen_is_point)
                        pass->pairing_func = (seer_is_point) ? space_see_point_point_self_see : space_see_nonpoint_nonpoint_self_see;
                    else
                        pass->pairing_func = (seer_is_point) ? space_see_point_nonpoint : space_see_nonpoint_point;
                }

                if (seen_space->type == TAY_CPU_SIMPLE)
                    pass->seen_func = cpu_simple_see_seen;
                else if (seen_space->type == TAY_CPU_KD_TREE)
                    pass->seen_func = cpu_kd_tree_see_seen;
                else if (seen_space->type == TAY_CPU_AABB_TREE)
                    pass->seen_func = cpu_aabb_tree_see_seen;
                else if (seen_space->type == TAY_CPU_GRID)
                    pass->seen_func = cpu_grid_see_seen;
                else if (seen_space->type == TAY_CPU_Z_GRID)
                    pass->seen_func = cpu_z_grid_see_seen;
                else {
                    tay_set_error2(state, TAY_ERROR_NOT_IMPLEMENTED, "unhandled CPU see pass seen type");
                    return 0;
                }
            }
        }
        else if (pass->type == TAY_PASS_ACT) {
            if (pass->act_group->ocl_enabled) {
                if (!ocl_get_pass_kernel(state, pass))
                    return 0;
            }
        }
    }

    /* prepare CPU spaces for simulation */
    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group) || group->ocl_enabled)
            continue;

        Space *space = &group->space;
        if (space->type == TAY_CPU_KD_TREE)
            cpu_tree_on_simulation_start(space);
        else if (space->type == TAY_CPU_GRID)
            cpu_grid_on_simulation_start(space);
        else if (space->type == TAY_CPU_Z_GRID)
            cpu_z_grid_on_simulation_start(space);
    }

    /* get built-in OCL kernels */
    if (ocl_groups_count)
        if (!ocl_grid_get_kernels(state))
            return 0;

    /* create OCL buffers */
    if (ocl_groups_count)
        if (!ocl_create_buffers(state))
            return 0;

    return 1;
}
