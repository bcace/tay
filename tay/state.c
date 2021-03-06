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
#if TAY_GPU
    s->gpu_shared.gpu = 0;
#endif
    return s;
}

static void _clear_group(TayGroup *group) {
    free(group->storage);
    group->storage = 0;
    group->first = 0;
}

static void _clear_space(Space *space) {
#if TAY_GPU
    if (space->type == ST_GPU_SIMPLE)
        gpu_simple_destroy_io_buffer(space);
#endif
    free(space->shared);
}

void tay_destroy_state(TayState *state) {
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        _clear_group(state->groups + i);
    for (int i = 0; i < TAY_MAX_SPACES; ++i)
        _clear_space(state->spaces + i);
#if TAY_GPU
    gpu_shared_destroy(state);
#endif
    free(state);
}

static SpaceType _translate_space_type(TaySpaceType type) {
    if (type == TAY_SPACE_CPU_SIMPLE)
        return ST_CPU_SIMPLE;
    else if (type == TAY_SPACE_CPU_TREE)
        return ST_CPU_TREE;
    else if (type == TAY_SPACE_CPU_GRID)
        return ST_CPU_GRID;
    else if (type == TAY_SPACE_GPU_SIMPLE_DIRECT)
        return ST_GPU_SIMPLE_DIRECT;
    else if (type == TAY_SPACE_GPU_SIMPLE_INDIRECT)
        return ST_GPU_SIMPLE_INDIRECT;
    else if (type == TAY_SPACE_CYCLE_ALL)
        return ST_CYCLE_ALL;
    return ST_NONE;
}

void tay_add_space(TayState *state, TaySpaceType space_type, int space_dims, float4 part_radii, int depth_correction, int shared_size_in_megabytes) {
    // ERROR: must be outside simulation
    assert(state->running == TAY_STATE_STATUS_IDLE);
    // ERROR: no more free spaces left
    assert(state->spaces_count < TAY_MAX_SPACES);

    Space *space = state->spaces + state->spaces_count++;
    space->type = _translate_space_type(space_type);
    space->depth_correction = depth_correction;
    space->radii = part_radii;
    space->dims = space_dims;
    space->shared_size = shared_size_in_megabytes * TAY_MB;
    space->shared = malloc(space->shared_size);
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        space->first[i] = 0;
        space->counts[i] = 0;
    }

    if (space->type & ST_GPU) {
#if TAY_GPU
        gpu_shared_create(state); /* initializes gpu support if not initialized already */
        gpu_simple_create_io_buffer(space, &state->gpu_shared);
#else
        // ERROR: gpu not supported
#endif
    }
}

int tay_add_group(TayState *state, int agent_size, int agent_capacity, int is_point, int space_index) {
    assert(agent_capacity > 0 && agent_capacity < TAY_MAX_AGENTS);
    int index = 0;
    for (; index < TAY_MAX_GROUPS; ++index)
        if (state->groups[index].storage == 0)
            break;
    assert(index < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + index;
    g->agent_size = agent_size;
    g->storage = calloc(agent_capacity, agent_size);
    g->capacity = agent_capacity;
    g->is_point = is_point;
    g->first = g->storage;
    g->space = state->spaces + space_index;
    TayAgentTag *prev = g->first;
    for (int i = 0; i < agent_capacity - 1; ++i) {
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
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    assert(g->first != 0);
    return g->first;
}

void tay_commit_available_agent(TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    assert(g->first != 0);
    TayAgentTag *a = g->first;
    g->first = a->next;
    space_add_agent(g->space, a, group);
}

void *tay_get_agent(TayState *state, int group, int index) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    return (char *)g->storage + g->agent_size * index;
}

void tay_simulation_start(TayState *state, const char *gpu_source) {
    // ERROR: this assert
    assert(state->running == TAY_STATE_STATUS_IDLE);

    state->running = TAY_STATE_STATUS_RUNNING;

    for (int i = 0; i < state->spaces_count; ++i) {
        Space *space = state->spaces + i;

        if (space->type == ST_CPU_TREE)
            cpu_tree_on_type_switch(space);
        else if (space->type == ST_CPU_GRID)
            cpu_grid_on_type_switch(space);
        else if (space->type & ST_GPU) {
#if TAY_GPU
            assert(gpu_source != 0); // ERROR: ...
            gpu_shared_refresh(state, gpu_source);
            gpu_shared_push_agents_and_pass_contexts(state);

            if (space->type == ST_GPU_SIMPLE)
                gpu_simple_fix_gpu_pointers(state);
#else
            // ERROR: gpu not supported
#endif
        }
    }
}

double tay_run(TayState *state, int steps) {
    assert(state->running == TAY_STATE_STATUS_RUNNING);

    /* start measuring run-time */
    struct timespec beg, end;
    timespec_get(&beg, TIME_UTC);

    /* run requested number of simulation steps */
    for (int step_i = 0; step_i < steps; ++step_i) {

        /* sort all agents into structures */
        for (int space_i = 0; space_i < state->spaces_count; ++space_i) {
            Space *space = state->spaces + space_i;

            switch (space->type) {
                case ST_CPU_SIMPLE: cpu_simple_sort(space, state->groups); break;
                case ST_CPU_TREE: cpu_tree_sort(space, state->groups); break;
                case ST_CPU_GRID: cpu_grid_sort(space, state->groups, state->passes, state->passes_count); break;
                default: assert(0);
            }
        }

        /* do passes */
        for (int pass_i = 0; pass_i < state->passes_count; ++pass_i) {
            TayPass *pass = state->passes + pass_i;

            if (pass->type == TAY_PASS_SEE) {
                Space *seer_space = state->groups[pass->seer_group].space;
                Space *seen_space = state->groups[pass->seen_group].space;

                if (seer_space == seen_space) { /* single space see */
                    Space *space = seer_space;

                    switch (space->type) {
                        case ST_CPU_SIMPLE: cpu_simple_single_space_see(space, pass); break;
                        case ST_CPU_TREE: cpu_tree_single_space_see(space, pass); break;
                        case ST_CPU_GRID: cpu_grid_single_space_see(space, pass); break;
                        default: assert(0); /* not implemented */
                    }
                }
                else { /* two space see */
                    assert(0); /* not implemented */
                }
            }
            else if (pass->type == TAY_PASS_ACT) {
                Space *space = state->groups[pass->act_group].space;

                /* act */
                switch (space->type) {
                    case ST_CPU_SIMPLE: cpu_simple_act(space, pass); break;
                    case ST_CPU_TREE: cpu_tree_act(space, pass); break;
                    case ST_CPU_GRID: cpu_grid_act(space, pass); break;
                    default: assert(0); /* not implemented */
                }
            }
            else
                assert(0); /* unhandled pass type */
        }

        /* return agents from structures */
        for (int space_i = 0; space_i < state->spaces_count; ++space_i) {
            Space *space = state->spaces + space_i;

            switch (space->type) {
                case ST_CPU_SIMPLE: cpu_simple_unsort(space, state->groups); break;
                case ST_CPU_TREE: cpu_tree_unsort(space, state->groups); break;
                case ST_CPU_GRID: cpu_grid_unsort(space, state->groups); break;
                case ST_GPU_SIMPLE: break;
                default: assert(0); /* not implemented */
            }
        }

#if TAY_TELEMETRY
        tay_threads_update_telemetry();
#endif
    }

    /* fetch agents from gpu */
#if TAY_GPU
    for (int space_i = 0; space_i < state->spaces_count; ++space_i) {
        Space *space = state->spaces + space_i;
        space_gpu_fetch_agents(state);
    }
#endif

    /* end measuring run-time */
    timespec_get(&end, TIME_UTC);
    double t = (end.tv_sec - beg.tv_sec) + ((long long)end.tv_nsec - (long long)beg.tv_nsec) * 1.0e-9;
    double ms = (t / (double)steps) * 1000.0;
    return ms;
}

void tay_simulation_end(TayState *state) {
    assert(state->running == TAY_STATE_STATUS_RUNNING);
    state->running = TAY_STATE_STATUS_IDLE;
}
