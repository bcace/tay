#include "space.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>


TayState *tay_create_state() {
    TayState *state = calloc(1, sizeof(TayState));
    state->is_running = 0;
    state->error = TAY_ERROR_NONE;
    state->ms_per_step = 0.0;
    state->next_group_id = 0;
    ocl_init(&state->ocl);
    return state;
}

static void _clear_space(Space *space) {
    free(space->shared);
    space->shared = 0;
    space->shared_size = 0;
}

static void _clear_group(TayGroup *group) {
    free(group->agent_storage[0]);
    free(group->agent_storage[1]);
    group->agent_storage[0] = group->storage = 0;
    group->agent_storage[1] = group->sort_storage = 0;
    _clear_space(&group->space);
}

void tay_destroy_state(TayState *state) {
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        _clear_group(state->groups + i);
    ocl_release_context(&state->ocl.device);
    free(state);
}

int tay_switch_to_ocl(TayState *state) {
    if (!state->ocl.device.enabled) {
        ocl_init_context(state, &state->ocl.device);
        state->recompile = 1;
    }
    return state->ocl.device.enabled;
}

int tay_switch_to_host(TayState *state) {
    if (state->ocl.device.enabled) {
        ocl_clear_program_and_buffers(state);
        ocl_release_context(&state->ocl.device);
        state->recompile = 1;
    }
    return state->ocl.device.enabled;
}

TayError tay_get_error(TayState *state) {
    return state->error;
}

void tay_set_error(TayState *state, TayError error) {
    fprintf(stderr, "ERROR:\n");
    state->error = error;
}

void tay_set_error2(TayState *state, TayError error, const char *message) {
    fprintf(stderr, "ERROR: %s\n", message);
    state->error = error;
}

TayGroup *tay_add_group(TayState *state, unsigned agent_size, unsigned agent_capacity, int is_point) {
    int group_i = 0;

    for (; group_i < TAY_MAX_GROUPS; ++group_i)
        if (group_is_inactive(state->groups + group_i))
            break;

    if (group_i == TAY_MAX_GROUPS) {
        tay_set_error(state, TAY_ERROR_GROUP_INDEX_OUT_OF_RANGE);
        return 0;
    }

    /* initialize group */
    TayGroup *group = state->groups + group_i;
    group->agent_size = agent_size;
    group->agent_storage[0] = group->storage = calloc(agent_capacity, agent_size);
    group->agent_storage[1] = group->sort_storage = calloc(agent_capacity, agent_size);
    group->capacity = agent_capacity;
    group->is_point = is_point;
    group->id = state->next_group_id++;

    /* initialize the group's space */
    Space *space = &group->space;
    space->type = TAY_CPU_SIMPLE;
    space->min_part_sizes = (float4){1.0f, 1.0f, 1.0f, 1.0f};
    space->dims = 3;
    space->shared_size = 0;
    space->shared = 0;
    space->count = 0;
    space->is_box_fixed = 0;

    space->ocl_common.space_buffer = 0;

    return group;
}

void tay_configure_space(TayState *state, TayGroup *group, TaySpaceType space_type, int space_dims, float4 min_part_sizes, int shared_size_in_megabytes) {
    Space *space = &group->space;
    space->type = space_type;
    space->min_part_sizes = min_part_sizes;
    space->dims = space_dims;
    space->shared_size = shared_size_in_megabytes * TAY_MB;
    space->shared = realloc(space->shared, space->shared_size);
    space->ocl_common.push_agents = 1;
    state->recompile = 1;
}

void tay_fix_space_box(TayState *state, TayGroup *group, float4 min, float4 max) {
    Space *space = &group->space;
    space->box.min = min;
    space->box.max = max;
    space->is_box_fixed = 1;
}

int group_is_active(TayGroup *group) {
    return group->storage != 0;
}

int group_is_inactive(TayGroup *group) {
    return group->storage == 0;
}

void tay_add_see(TayState *state, TayGroup *seer_group, TayGroup *seen_group, TAY_SEE_FUNC func, char *func_name, float4 radii, int self_see, void *context, unsigned context_size) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_SEE;
    p->context = context;
    p->context_size = context_size;
    p->see = func;
    p->radii = radii;
    p->self_see = self_see;
    p->seer_group = seer_group;
    p->seen_group = seen_group;
    strcpy_s(p->func_name, TAY_MAX_FUNC_NAME, func_name);
}

void tay_add_act(TayState *state, TayGroup *act_group, TAY_ACT_FUNC func, char *func_name, void *context, unsigned context_size) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_ACT;
    p->context = context;
    p->context_size = context_size;
    p->act = func;
    p->act_group = act_group;
    strcpy_s(p->func_name, TAY_MAX_FUNC_NAME, func_name);
}

void tay_clear_all_agents(TayState *state, TayGroup *group) {
    group->space.count = 0;
    if (state->ocl.device.enabled)
        group->space.ocl_common.push_agents = 1;
}

void *tay_get_available_agent(TayState *state, TayGroup *group) {
    // ERROR: check group
    Space *space = &group->space;
    return group->storage + group->agent_size * space->count;
}

void tay_commit_available_agent(TayState *state, TayGroup *group) {
    // ERROR: check group
    Space *space = &group->space;
    ++space->count;
}

void *tay_get_agent(TayState *state, TayGroup *group, int index) {
    // ERROR: check group
    return group->storage + group->agent_size * index;
}

void tay_simulation_start(TayState *state) {
    if (state->is_running) {
        tay_set_error2(state, TAY_ERROR_STATE_STATUS, "cannot start simulation if it's already running");
        return;
    }

    state->recompile = 1;
    state->is_running = 1;
}

/* Returns the number of steps executed */
int tay_run(TayState *state, int steps) {

    if (!state_compile(state))
        return 0;

    if (!state->is_running) {
        tay_set_error2(state, TAY_ERROR_STATE_STATUS, "cannot run simulation steps if simulation is not started");
        return 0;
    }

    /* start measuring run-time */
    struct timespec beg, end;
    timespec_get(&beg, TIME_UTC);

    /* ocl groups and passes setup */
    if (state->ocl.device.enabled) {

        /* push agents if they have been modified (push_agents flag) */
        ocl_push_agents_non_blocking(state);

        /* push pass contexts */
        ocl_push_pass_contexts_non_blocking(state);

        ocl_finish(state);
    }

    /* run requested number of simulation steps */
    for (int step_i = 0; step_i < steps; ++step_i) {

        /* sort all agents into structures */
        for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
            TayGroup *group = state->groups + i;

            if (group_is_inactive(group))
                continue;

            if (state->ocl.device.enabled) {
                if (group->space.type == TAY_CPU_Z_GRID)
                    ocl_grid_run_sort_kernel(state, group);
            }
            else {
                if (group->space.type == TAY_CPU_KD_TREE)
                    cpu_tree_sort(group);
                else if (group->space.type == TAY_CPU_AABB_TREE)
                    cpu_aabb_tree_sort(group);
                else if (group->space.type == TAY_CPU_GRID)
                    cpu_grid_sort(group);
                else if (group->space.type == TAY_CPU_Z_GRID)
                    cpu_z_grid_sort(group);
            }
        }

        /* do passes */
        for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
            TayPass *pass = state->passes + pass_i;

            if (pass->type == TAY_PASS_SEE) {
                Space *seer_space = &pass->seer_group->space;

                if (state->ocl.device.enabled)
                    ocl_run_see_kernel(state, pass);
                else if (seer_space->type == TAY_CPU_SIMPLE)
                    cpu_simple_see(pass);
                else if (seer_space->type == TAY_CPU_KD_TREE)
                    cpu_tree_see(pass);
                else if (seer_space->type == TAY_CPU_AABB_TREE)
                    cpu_aabb_tree_see(pass);
                else if (seer_space->type == TAY_CPU_GRID)
                    cpu_grid_see(pass);
                else if (seer_space->type == TAY_CPU_Z_GRID)
                    cpu_z_grid_see(pass);
                else {
                    tay_set_error(state, TAY_ERROR_NOT_IMPLEMENTED);
                    return 0;
                }
            }
            else if (pass->type == TAY_PASS_ACT) {
                Space *act_space = &pass->act_group->space;

                if (state->ocl.device.enabled)
                    ocl_run_act_kernel(state, pass);
                else if (act_space->type == TAY_CPU_SIMPLE ||
                    act_space->type == TAY_CPU_KD_TREE ||
                    act_space->type == TAY_CPU_AABB_TREE ||
                    act_space->type == TAY_CPU_GRID ||
                    act_space->type == TAY_CPU_Z_GRID) {
                    space_act(pass);
                }
                else {
                    tay_set_error(state, TAY_ERROR_NOT_IMPLEMENTED);
                    return 0;
                }
            }
        }

        /* return agents from structures */
        for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
            TayGroup *group = state->groups + i;

            if (group_is_inactive(group))
                continue;

            if (state->ocl.device.enabled) {
                if (group->space.type == TAY_CPU_Z_GRID)
                    ocl_grid_run_unsort_kernel(state, group);
            }
            else {
                if (group->space.type == TAY_CPU_GRID)
                    cpu_grid_unsort(group);
                else if (group->space.type == TAY_CPU_Z_GRID)
                    cpu_z_grid_unsort(group);
            }
        }

#if TAY_TELEMETRY
        tay_threads_update_telemetry();
#endif
    }

    /* fetch agents from GPU */
    if (state->ocl.device.enabled)
        ocl_fetch_agents(state);

    /* end measuring run-time */
    timespec_get(&end, TIME_UTC);
    double t = (end.tv_sec - beg.tv_sec) + ((long long)end.tv_nsec - (long long)beg.tv_nsec) * 1.0e-9;
    state->ms_per_step = (t / (double)steps) * 1000.0;

    return steps;
}

void tay_simulation_end(TayState *state) {

    if (!state->is_running) {
        tay_set_error2(state, TAY_ERROR_STATE_STATUS, "cannot end simulationif it's not running");
        return;
    }

    state->is_running = 0;
    ocl_clear_program_and_buffers(state);
}

double tay_get_ms_per_step_for_last_run(TayState *state) {
    return state->ms_per_step;
}
