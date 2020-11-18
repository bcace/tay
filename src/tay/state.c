#include "space.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>


TayState *tay_create_state(int space_dims, float4 see_radii) {
    return tay_create_state_specific(space_dims, see_radii, ST_ADAPTIVE, 0);
}

TayState *tay_create_state_specific(int space_dims, float4 see_radii, int initial_space_type, int depth_correction) {
    TayState *s = calloc(1, sizeof(TayState));
    s->running = TAY_STATE_STATUS_IDLE;
    s->source = 0;

    SpaceType space_type = ST_ADAPTIVE;

    if (initial_space_type == TAY_SPACE_CPU_SIMPLE)
        space_type = ST_CPU_SIMPLE;
    else if (initial_space_type == TAY_SPACE_CPU_TREE)
        space_type = ST_CPU_TREE;
    else if (initial_space_type == TAY_SPACE_GPU_SIMPLE)
        space_type = ST_GPU_SIMPLE;
    else if (initial_space_type == TAY_SPACE_GPU_TREE)
        space_type = ST_GPU_TREE;

    space_init(&s->space, space_dims, see_radii, depth_correction, space_type);

    return s;
}

static void _clear_group(TayGroup *group) {
    free(group->storage);
    group->storage = 0;
    group->first = 0;
}

void tay_destroy_state(TayState *state) {
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        _clear_group(state->groups + i);
    free(state);
}

void tay_set_source(TayState *state, const char *source) {
    state->source = source;
}

int tay_add_group(TayState *state, int agent_size, int agent_capacity) {
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
    g->is_point = 1;
    g->first = g->storage;
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
    space_add_agent(&state->space, a, group);
}

void *tay_get_agent(TayState *state, int group, int index) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    return (char *)g->storage + g->agent_size * index;
}

void tay_simulation_start(TayState *state) {
    assert(state->running == TAY_STATE_STATUS_IDLE);
    state->running = TAY_STATE_STATUS_RUNNING;
    space_on_simulation_start(state);
}

void tay_run(TayState *state, int steps) {
    assert(state->running == TAY_STATE_STATUS_RUNNING);

    struct timespec beg, end;
    timespec_get(&beg, TIME_UTC);

#if TAY_INSTRUMENT
    tay_runner_reset_stats();
#endif

    space_run(state, steps);

    timespec_get(&end, TIME_UTC);
    double t = (end.tv_sec - beg.tv_sec) + ((long long)end.tv_nsec - (long long)beg.tv_nsec) * 1.0e-9;
    double fps = steps / t;

#if TAY_INSTRUMENT
    tay_runner_report_stats();
#endif

    printf("run time: %g sec, %g fps\n\n", t, fps);
}

void tay_simulation_end(TayState *state) {
    assert(state->running == TAY_STATE_STATUS_RUNNING);
    state->running = TAY_STATE_STATUS_IDLE;
    space_on_simulation_end(state);
}

int group_tag_to_index(TayGroup *group, TayAgentTag *tag) {
    return (tag != 0) ? (int)((char *)tag - (char *)group->storage) / group->agent_size : -1;
}
