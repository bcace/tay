#include "space.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>


TayState *tay_create_state() {
    TayState *s = calloc(1, sizeof(TayState));
    s->running = TAY_STATE_STATUS_IDLE;
    s->error = TAY_ERROR_NONE;
    s->ms_per_step = 0.0;
    return s;
}

static void _clear_space(Space *space) {
    free(space->shared);
    space->shared = 0;
    space->shared_size = 0;
}

static void _clear_group(TayGroup *group) {
    free(group->storage);
    group->storage = 0;
    group->first = 0;
    _clear_space(&group->space);
}

void tay_destroy_state(TayState *state) {
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        _clear_group(state->groups + i);
    free(state);
}

TayError tay_get_error(TayState *state) {
    return state->error;
}

void state_set_error(TayState *state, TayError error) {
    state->error = error;
}

TaySpaceDesc tay_space_desc(TaySpaceType space_type, int space_dims, float4 part_radii, int shared_size_in_megabytes) {
    TaySpaceDesc desc;
    desc.space_type = space_type;
    desc.space_dims = space_dims;
    desc.part_radii = part_radii;
    desc.shared_size_in_megabytes = shared_size_in_megabytes;
    return desc;
}

TayGroup *tay_add_group(TayState *state, unsigned agent_size, unsigned agent_capacity, int is_point, TaySpaceDesc space_desc) {
    int group_i = 0;

    for (; group_i < TAY_MAX_GROUPS; ++group_i)
        if (group_is_inactive(state->groups + group_i))
            break;

    if (group_i == TAY_MAX_GROUPS) {
        state_set_error(state, TAY_ERROR_GROUP_INDEX_OUT_OF_RANGE);
        return 0;
    }

    if (is_point) {
        if (space_desc.space_type == TAY_CPU_AABB_TREE) {
            state_set_error(state, TAY_ERROR_POINT_NONPOINT_MISMATCH);
            return 0;
        }
    }
    else {
        if (space_desc.space_type == TAY_CPU_GRID) {
            state_set_error(state, TAY_ERROR_POINT_NONPOINT_MISMATCH);
            return 0;
        }
    }

    /* initialize group */
    TayGroup *group = state->groups + group_i;
    group->agent_size = agent_size;
    group->storage = calloc(agent_capacity, agent_size);
    group->capacity = agent_capacity;
    group->is_point = is_point;
    group->first = group->storage;

    /* connect all dead agents in storage into a list */
    TayAgentTag *prev = group->first;
    for (unsigned i = 0; i < agent_capacity - 1; ++i) {
        TayAgentTag *next = (TayAgentTag *)((char *)prev + agent_size);
        prev->next = next;
        prev = next;
    }
    prev->next = 0;

    /* initialize the group's space */
    Space *space = &group->space;
    space->type = space_desc.space_type;
    space->radii = space_desc.part_radii;
    space->dims = space_desc.space_dims;
    space->shared_size = space_desc.shared_size_in_megabytes * TAY_MB;
    space->shared = malloc(space->shared_size);
    space->first = 0;
    space->count = 0;

    return group;
}

int group_is_active(TayGroup *group) {
    return group->storage != 0;
}

int group_is_inactive(TayGroup *group) {
    return group->storage == 0;
}

void tay_add_see(TayState *state, TayGroup *seer_group, TayGroup *seen_group, TAY_SEE_FUNC func, const char *func_name, float4 radii, void *context, int context_size) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_SEE;
    p->context = context;
    p->context_size = context_size;
    p->see = func;
    p->func_name = func_name;
    p->radii = radii;
    p->seer_group = seer_group;
    p->seen_group = seen_group;
}

void tay_add_act(TayState *state, TayGroup *act_group, TAY_ACT_FUNC func, const char *func_name, void *context, int context_size) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_ACT;
    p->context = context;
    p->context_size = context_size;
    p->act = func;
    p->func_name = func_name;
    p->act_group = act_group;
}

void *tay_get_available_agent(TayState *state, TayGroup *group) {
    // ERROR: check group
    assert(group->first != 0);
    return group->first;
}

void tay_commit_available_agent(TayState *state, TayGroup *group) {
    // ERROR: check group
    assert(group->first != 0);

    /* remove agent from storage */
    TayAgentTag *a = group->first;
    group->first = a->next;

    /* add agent to space (make it live) */
    {
        Space *space = &group->space;
        a->next = space->first;
        space->first = a;
        box_update_from_agent(&space->box, a, space->dims, group->is_point);
        ++space->count;
    }
}

void *tay_get_agent(TayState *state, TayGroup *group, int index) {
    // ERROR: check group
    return (char *)group->storage + group->agent_size * index;
}

void tay_simulation_start(TayState *state) {
    assert(state->running == TAY_STATE_STATUS_IDLE); // ERROR: this assert
    state->running = TAY_STATE_STATUS_RUNNING;

    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        Space *space = &group->space;
        if (space->type == TAY_CPU_TREE)
            cpu_tree_on_simulation_start(space);
        else if (space->type == TAY_CPU_GRID)
            cpu_grid_on_simulation_start(space);
    }
}

static SEE_PAIRING_FUNC _get_many_to_many_pairing_function(int seer_is_point, int seen_is_point) {
    if (seer_is_point == seen_is_point)
        return (seer_is_point) ? space_see_point_point : space_see_nonpoint_nonpoint;
    else
        return (seer_is_point) ? space_see_point_nonpoint : space_see_nonpoint_point;
}

static SEE_PAIRING_FUNC _get_one_to_many_pairing_function(int seer_is_point, int seen_is_point) {
    if (seer_is_point == seen_is_point)
        return (seer_is_point) ? space_see_one_to_many_point_to_point : space_see_one_to_many_nonpoint_to_nonpoint;
    else
        return (seer_is_point) ? space_see_one_to_many_point_to_nonpoint : space_see_one_to_many_nonpoint_to_point;
}

static TayError _compile_passes(TayState *state) {

    for (int pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;
            int seer_is_point = pass->seer_group->is_point;
            int seen_is_point = pass->seen_group->is_point;

            pass->seer_space = seer_space;
            pass->seen_space = seen_space;

            if (seer_space == seen_space) {

                if (seer_space->type == TAY_CPU_SIMPLE) {
                    pass->pairing_func = _get_many_to_many_pairing_function(seer_is_point, seen_is_point);
                    pass->exec_func = cpu_simple_see;
                }
                else if (seer_space->type == TAY_CPU_TREE) {
                    pass->pairing_func = _get_many_to_many_pairing_function(seer_is_point, seen_is_point);
                    pass->exec_func = cpu_tree_see;
                }
                else if (seer_space->type == TAY_CPU_GRID) {
                    pass->pairing_func = _get_one_to_many_pairing_function(seer_is_point, seen_is_point);
                    pass->exec_func = cpu_grid_see;
                }
                else if (seer_space->type == TAY_CPU_AABB_TREE) {
                    pass->pairing_func = _get_many_to_many_pairing_function(seer_is_point, seen_is_point);
                    pass->exec_func = cpu_aabb_tree_see;
                }
                else
                    return TAY_ERROR_NOT_IMPLEMENTED;
            }
            else if (seer_space->dims == seen_space->dims) {

                if (seer_space->type == seen_space->type) {

                    if (seer_space->type == TAY_CPU_SIMPLE) {
                        pass->pairing_func = _get_many_to_many_pairing_function(seer_is_point, seen_is_point);
                        pass->exec_func = cpu_simple_see;
                    }
                    else if (seer_space->type == TAY_CPU_TREE) {
                        pass->pairing_func = _get_many_to_many_pairing_function(seer_is_point, seen_is_point);
                        pass->exec_func = cpu_tree_see;
                    }
                    else
                        return TAY_ERROR_NOT_IMPLEMENTED;
                }
                else
                    return TAY_ERROR_NOT_IMPLEMENTED;
            }
            else
                return TAY_ERROR_NOT_IMPLEMENTED;
        }
        else if (pass->type == TAY_PASS_ACT) {
            Space *act_space = &pass->act_group->space;
            int act_is_point = pass->act_group->is_point;

            pass->act_space = act_space;

            if (act_space->type == TAY_CPU_SIMPLE)
                pass->exec_func = cpu_simple_act;
            else if (act_space->type == TAY_CPU_TREE)
                pass->exec_func = cpu_tree_act;
            else if (act_space->type == TAY_CPU_GRID)
                pass->exec_func = cpu_grid_act;
            else if (act_space->type == TAY_CPU_AABB_TREE)
                pass->exec_func = cpu_aabb_tree_act;
            else
                return TAY_ERROR_NOT_IMPLEMENTED;
        }
    }

    return TAY_ERROR_NONE;
}

/* Returns the number of steps executed */
int tay_run(TayState *state, int steps) {

    if (state->running != TAY_STATE_STATUS_RUNNING) {
        state_set_error(state, TAY_ERROR_STATE_STATUS);
        return 0;
    }

    TayError error = _compile_passes(state);
    if (error) {
        state_set_error(state, error);
        return 0;
    }

    /* start measuring run-time */
    struct timespec beg, end;
    timespec_get(&beg, TIME_UTC);

    /* run requested number of simulation steps */
    for (int step_i = 0; step_i < steps; ++step_i) {

        /* sort all agents into structures */
        for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
            TayGroup *group = state->groups + i;

            if (group_is_inactive(group))
                continue;

            switch (group->space.type) {
                case TAY_CPU_SIMPLE: cpu_simple_sort(group); break;
                case TAY_CPU_TREE: cpu_tree_sort(group); break;
                case TAY_CPU_GRID: cpu_grid_sort(group, state->passes, state->passes_count); break;
                case TAY_CPU_AABB_TREE: cpu_aabb_tree_sort(group); break;
                default: assert(0);
            }
        }

        /* do passes */
        for (int pass_i = 0; pass_i < state->passes_count; ++pass_i) {
            TayPass *pass = state->passes + pass_i;
            pass->exec_func(pass);
        }

        /* return agents from structures */
        for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
            TayGroup *group = state->groups + i;

            if (group_is_inactive(group))
                continue;

            switch (group->space.type) {
                case TAY_CPU_SIMPLE: cpu_simple_unsort(group); break;
                case TAY_CPU_TREE: cpu_tree_unsort(group); break;
                case TAY_CPU_GRID: cpu_grid_unsort(group); break;
                case TAY_CPU_AABB_TREE: cpu_aabb_tree_unsort(group); break;
                default: assert(0); /* not implemented */
            }
        }

#if TAY_TELEMETRY
        tay_threads_update_telemetry();
#endif
    }

    /* end measuring run-time */
    timespec_get(&end, TIME_UTC);
    double t = (end.tv_sec - beg.tv_sec) + ((long long)end.tv_nsec - (long long)beg.tv_nsec) * 1.0e-9;
    state->ms_per_step = (t / (double)steps) * 1000.0;

    return steps;
}

void tay_simulation_end(TayState *state) {
    assert(state->running == TAY_STATE_STATUS_RUNNING);
    state->running = TAY_STATE_STATUS_IDLE;
}

double tay_get_ms_per_step_for_last_run(TayState *state) {
    return state->ms_per_step;
}
