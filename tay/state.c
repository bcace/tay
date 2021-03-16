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
    s->spaces_count = 0;
    s->error = TAY_ERROR_NONE;
    s->ms_per_step = 0.0;
    return s;
}

static void _clear_group(TayGroup *group) {
    free(group->storage);
    group->storage = 0;
    group->first = 0;
}

static void _clear_space(Space *space) {
    free(space->shared);
}

void tay_destroy_state(TayState *state) {
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        _clear_group(state->groups + i);
    for (int i = 0; i < TAY_MAX_SPACES; ++i)
        _clear_space(state->spaces + i);
    free(state);
}

TayError tay_get_error(TayState *state) {
    return state->error;
}

void state_set_error(TayState *state, TayError error) {
    state->error = error;
}

void tay_add_space(TayState *state, TaySpaceType space_type, int space_dims, float4 part_radii, int shared_size_in_megabytes) {
    // ERROR: must be outside simulation
    assert(state->running == TAY_STATE_STATUS_IDLE);
    // ERROR: no more free spaces left
    assert(state->spaces_count < TAY_MAX_SPACES);

    Space *space = state->spaces + state->spaces_count++;
    space->type = space_type;
    space->radii = part_radii;
    space->dims = space_dims;
    space->shared_size = shared_size_in_megabytes * TAY_MB;
    space->shared = malloc(space->shared_size);

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        space->first[i] = 0;
        space->counts[i] = 0;
    }
}

int tay_add_group(TayState *state, unsigned agent_size, unsigned agent_capacity, int is_point, unsigned space_index) {

    if (space_index >= state->spaces_count) {
        state_set_error(state, TAY_ERROR_SPACE_INDEX_OUT_OF_RANGE);
        return -1;
    }

    Space *space = state->spaces + space_index;
    int index = 0;

    for (; index < TAY_MAX_GROUPS; ++index)
        if (state->groups[index].storage == 0)
            break;

    if (index == TAY_MAX_GROUPS) {
        state_set_error(state, TAY_ERROR_GROUP_INDEX_OUT_OF_RANGE);
        return -1;
    }

    TayGroup *g = state->groups + index;
    g->agent_size = agent_size;
    g->storage = calloc(agent_capacity, agent_size);
    g->capacity = agent_capacity;
    g->is_point = is_point;
    g->first = g->storage;
    g->space = space;

    TayAgentTag *prev = g->first;
    for (unsigned i = 0; i < agent_capacity - 1; ++i) {
        TayAgentTag *next = (TayAgentTag *)((char *)prev + agent_size);
        prev->next = next;
        prev = next;
    }
    prev->next = 0;

    return index;
}

void tay_add_see(TayState *state, int seer_group, int seen_group, TAY_SEE_FUNC func, const char *func_name, float4 radii, void *context, int context_size) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_SEE;
    p->context = context;
    p->context_size = context_size;
    p->see = func;
    p->func_name = func_name;
    p->seer_group = seer_group;
    p->seen_group = seen_group;
    p->radii = radii;
}

void tay_add_act(TayState *state, int act_group, TAY_ACT_FUNC func, const char *func_name, void *context, int context_size) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_ACT;
    p->context = context;
    p->context_size = context_size;
    p->act = func;
    p->func_name = func_name;
    p->act_group = act_group;
}

void *tay_get_available_agent(TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS); // ERROR: this assert
    TayGroup *g = state->groups + group;
    assert(g->first != 0);
    return g->first;
}

void tay_commit_available_agent(TayState *state, int group_i) {
    assert(group_i >= 0 && group_i < TAY_MAX_GROUPS); // ERROR: this assert
    TayGroup *group = state->groups + group_i;
    assert(group->first != 0);

    /* remove agent from storage */
    TayAgentTag *a = group->first;
    group->first = a->next;

    /* add agent to space (make it live) */
    {
        Space *space = group->space;
        a->next = space->first[group_i];
        space->first[group_i] = a;
        box_update_from_agent(&space->box, a, space->dims, group->is_point);
        ++space->counts[group_i];
    }
}

void *tay_get_agent(TayState *state, int group, int index) {
    assert(group >= 0 && group < TAY_MAX_GROUPS); // ERROR: this assert
    TayGroup *g = state->groups + group;
    return (char *)g->storage + g->agent_size * index;
}

void tay_simulation_start(TayState *state) {
    assert(state->running == TAY_STATE_STATUS_IDLE); // ERROR: this assert
    state->running = TAY_STATE_STATUS_RUNNING;

    for (unsigned i = 0; i < state->spaces_count; ++i) {
        Space *space = state->spaces + i;

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
            Space *seer_space = state->groups[pass->seer_group].space;
            Space *seen_space = state->groups[pass->seen_group].space;
            int seer_is_point = state->groups[pass->seer_group].is_point;
            int seen_is_point = state->groups[pass->seen_group].is_point;

            if (seer_space == seen_space) {
                TaySpaceType space_type = seer_space->type;

                pass->seer_space = seer_space;
                pass->seen_space = seen_space;

                if (space_type == TAY_CPU_SIMPLE) {
                    pass->pairing_func = _get_many_to_many_pairing_function(seer_is_point, seen_is_point);
                    pass->exec_func = cpu_simple_see;
                }
                else if (space_type == TAY_CPU_TREE) {
                    pass->pairing_func = _get_many_to_many_pairing_function(seer_is_point, seen_is_point);
                    pass->exec_func = cpu_tree_see;
                }
                else if (space_type == TAY_CPU_GRID) {

                    if (!seer_is_point || !seen_is_point) /* grid cannot handle non-point agents */
                        return TAY_ERROR_POINT_NONPOINT_MISMATCH;

                    pass->pairing_func = _get_one_to_many_pairing_function(seer_is_point, seen_is_point);
                    pass->exec_func = cpu_grid_see;
                }
                else if (space_type == TAY_CPU_AABB_TREE) {

                    if (seer_is_point || seen_is_point) /* aabb tree cannot handle point agents */
                        return TAY_ERROR_POINT_NONPOINT_MISMATCH;

                    pass->pairing_func = _get_many_to_many_pairing_function(seer_is_point, seen_is_point);
                    pass->exec_func = cpu_aabb_tree_see;
                }
                else
                    return TAY_ERROR_NOT_IMPLEMENTED;
            }
            else
                return TAY_ERROR_NOT_IMPLEMENTED;
        }
        else if (pass->type == TAY_PASS_ACT) {
            Space *act_space = state->groups[pass->act_group].space;
            int act_is_point = state->groups[pass->act_group].is_point;

            pass->act_space = act_space;

            if (act_space->type == TAY_CPU_SIMPLE)
                pass->exec_func = cpu_simple_act;
            else if (act_space->type == TAY_CPU_TREE)
                pass->exec_func = cpu_tree_act;
            else if (act_space->type == TAY_CPU_GRID) {

                if (!act_is_point) /* grid cannot handle non-point agents */
                    return TAY_ERROR_POINT_NONPOINT_MISMATCH;

                pass->exec_func = cpu_grid_act;
            }
            else if (act_space->type == TAY_CPU_AABB_TREE) {

                if (act_is_point) /* aabb tree cannot handle point agents */
                    return TAY_ERROR_POINT_NONPOINT_MISMATCH;

                pass->exec_func = cpu_aabb_tree_act;
            }
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
        for (unsigned space_i = 0; space_i < state->spaces_count; ++space_i) {
            Space *space = state->spaces + space_i;

            switch (space->type) {
                case TAY_CPU_SIMPLE: cpu_simple_sort(space, state->groups); break;
                case TAY_CPU_TREE: cpu_tree_sort(space, state->groups); break;
                case TAY_CPU_GRID: cpu_grid_sort(space, state->groups, state->passes, state->passes_count); break;
                case TAY_CPU_AABB_TREE: cpu_aabb_tree_sort(space, state->groups); break;
                default: assert(0);
            }
        }

        /* do passes */
        for (int pass_i = 0; pass_i < state->passes_count; ++pass_i) {
            TayPass *pass = state->passes + pass_i;
            pass->exec_func(pass);
        }

        /* return agents from structures */
        for (unsigned space_i = 0; space_i < state->spaces_count; ++space_i) {
            Space *space = state->spaces + space_i;

            switch (space->type) {
                case TAY_CPU_SIMPLE: cpu_simple_unsort(space, state->groups); break;
                case TAY_CPU_TREE: cpu_tree_unsort(space, state->groups); break;
                case TAY_CPU_GRID: cpu_grid_unsort(space, state->groups); break;
                case TAY_CPU_AABB_TREE: cpu_aabb_tree_unsort(space, state->groups); break;
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
