#include "state.h"
#include "space.h"
#include "thread.h"


static inline unsigned _powu(unsigned v, unsigned e) {
    unsigned r = v;
    for (unsigned i = 1; i < e; ++i)
        r *= v;
    return r;
}

int state_compile(TayState *state) {
    int has_ocl_work = 0;

    /* check basic group settings */
    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group->is_point) {
            if (group->space.type == TAY_CPU_AABB_TREE) {
                tay_set_error2(state, TAY_ERROR_OCL, "aabb tree structure can only contain non-point agents");
                return 0;
            }
        }
        else {
            if (group->space.type == TAY_CPU_GRID || group->space.type == TAY_CPU_Z_GRID) {
                tay_set_error2(state, TAY_ERROR_OCL, "grid structure can only contain point agents");
                return 0;
            }
        }
    }

    /* check basic pass settings */
    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        int is_ocl_pass = 0;

        if (pass->type == TAY_PASS_SEE) {
            if (pass->is_pic) {
                if (runner.thread_storage_size < _powu(sizeof(void *), 4)) { // TODO: fix dims argument of _powu, the worst case scenario
                    tay_set_error2(state, TAY_ERROR_PIC, "thread storage size is not large enough for PIC kernel");
                    return 0;
                }
            }
            else if (pass->seer_group->ocl_enabled && pass->seen_group->ocl_enabled) {
                is_ocl_pass = 1;
            }
            else {
                if (pass->see == 0) {
                    tay_set_error2(state, TAY_ERROR_OCL, "cpu pass with no see func set");
                    return 0;
                }
            }
        }
        else if (pass->type == TAY_PASS_ACT) {
            if (pass->is_pic) {
                // ...
            }
            else if (pass->act_group->ocl_enabled) {
                is_ocl_pass = 1;
            }
            else {
                if (pass->see == 0) {
                    tay_set_error2(state, TAY_ERROR_OCL, "cpu pass with no act func set");
                    return 0;
                }
            }
        }

        if (is_ocl_pass) {
            if (pass->context != 0 && pass->context_size == 0) {
                tay_set_error2(state, TAY_ERROR_OCL, "ocl pass context set but context size is 0");
                return 0;
            }
            else if (pass->context == 0 && pass->context_size != 0) {
                tay_set_error2(state, TAY_ERROR_OCL, "ocl pass context not set but context size is not 0");
                return 0;
            }

            if (pass->func_name[0] == '\0') {
                tay_set_error2(state, TAY_ERROR_OCL, "ocl pass with no func name set");
                return 0;
            }

            has_ocl_work = 1;
        }
    }

    /* initialize ocl context if there's something to be done on the GPU and it was not yet initialized */
    if (has_ocl_work && !state->ocl.device.enabled) {
        if (!ocl_init_context(state, &state->ocl.device))
            return 0;
    }

    /* compose and compile OCL program */
    if (has_ocl_work)
        if (!ocl_compile_program(state))
            return 0;

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

            if (!pass->is_pic && seer_space->dims != seen_space->dims) {
                tay_set_error2(state, TAY_ERROR_NOT_IMPLEMENTED, "groups with different numbers of dimensions not supported");
                return 0;
            }

            if (pass->is_pic) {
                // ...
            }
            else if (seer_is_ocl && seen_is_ocl) { /* both groups must be ocl enabled to have an ocl pass */
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
            if (pass->is_pic) {
                // ...
            }
            else if (pass->act_group->ocl_enabled) {
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
    if (has_ocl_work)
        if (!ocl_grid_get_kernels(state))
            return 0;

    /* create OCL buffers */
    if (has_ocl_work)
        if (!ocl_create_buffers(state))
            return 0;

    return 1;
}
