#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>


TayState *tay_create_state(TaySpaceType space_type, int space_dims, float *see_radii, int max_depth_correction) {
    assert(sizeof(TayAgentTag) == TAY_AGENT_TAG_SIZE);
    TayState *s = calloc(1, sizeof(TayState));
    if (space_type == TAY_SPACE_SIMPLE)
        space_simple_init(&s->space, space_dims);
    else if (space_type == TAY_SPACE_TREE)
        space_tree_init(&s->space, space_dims, see_radii, max_depth_correction);
    else if (space_type == TAY_SPACE_GPU_SIMPLE)
        space_gpu_simple_init(&s->space, space_dims);
    else
        assert(0); /* not implemented */
    s->running = TAY_STATE_STATUS_IDLE;
    return s;
}

static void _clear_group(TayGroup *group) {
    free(group->storage);
    group->storage = 0;
    group->first = 0;
}

void tay_destroy_state(TayState *state) {
    state->space.destroy(&state->space);
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        _clear_group(state->groups + i);
    free(state);
}

int tay_add_group(TayState *state, int agent_size, int agent_capacity) {
    assert(agent_capacity != 0);
    int index = 0;
    for (; index < TAY_MAX_GROUPS; ++index)
        if (state->groups[index].storage == 0)
            break;
    assert(index < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + index;
    int agent_size_with_tag = agent_size + sizeof(TayAgentTag);
    g->agent_size = agent_size;
    g->agent_size_with_tag = agent_size_with_tag;
    g->storage = calloc(agent_capacity, agent_size_with_tag);
    g->capacity = agent_capacity;
    g->first = g->storage;
    TayAgentTag *prev = g->first;
    for (int i = 0; i < agent_capacity - 1; ++i) {
        TayAgentTag *next = (TayAgentTag *)((char *)prev + agent_size_with_tag);
        prev->next = next;
        prev = next;
    }
    prev->next = 0;
    return index;
}

void tay_add_see(TayState *state, int seer_group, int seen_group, void (*func)(void *, void *, struct TayThreadContext *), float *radii, void *context) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_SEE;
    p->context = context;
    p->see = func;
    p->seer_group = seer_group;
    p->seen_group = seen_group;
    for (int i = 0; i < TAY_MAX_DIMENSIONS; ++i)
        p->radii[i] = radii[i];
}

void tay_add_act(TayState *state, int act_group, void (*func)(void *, struct TayThreadContext *), void *context) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_ACT;
    p->context = context;
    p->act = func;
    p->act_group = act_group;
}

void *tay_get_available_agent(TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    assert(g->first != 0);
    return TAY_AGENT_DATA(g->first);
}

void tay_commit_available_agent(TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    assert(g->first != 0);
    TayAgentTag *a = g->first;
    g->first = a->next;
    int index = (int)((char *)a - (char *)g->storage) / g->agent_size_with_tag;
    state->space.add(&state->space, a, group, index);
}

void *tay_get_agent(TayState *state, int group, int index) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    return (char *)g->storage + (sizeof(TayAgentTag) + g->agent_size) * index + sizeof(TayAgentTag);
}

void *tay_get_agent_tag(TayState *state, int group, int index) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    return (char *)g->storage + (sizeof(TayAgentTag) + g->agent_size) * index;
}

void tay_simulation_start(TayState *state) {
    assert(state->running == TAY_STATE_STATUS_IDLE);
    state->running = TAY_STATE_STATUS_RUNNING;
    if (state->space.on_simulation_start)
        state->space.on_simulation_start(&state->space, state);
}

void tay_run(TayState *state, int steps) {
    assert(state->running == TAY_STATE_STATUS_RUNNING);

    struct timespec beg, end;
    timespec_get(&beg, TIME_UTC);

    tay_runner_reset_stats();

    for (int i = 0; i < steps; ++i) {

        if (state->space.update)
            state->space.update(&state->space);

        for (int j = 0; j < state->passes_count; ++j) {
            TayPass *p = state->passes + j;
            if (p->type == TAY_PASS_SEE)
                state->space.see(&state->space, p);
            else if (p->type == TAY_PASS_ACT)
                state->space.act(&state->space, p);
            else
                assert(0); /* not implemented */
        }
    }

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
    if (state->space.on_simulation_end)
        state->space.on_simulation_end(&state->space);
}

void space_init(TaySpace *space,
                void *storage,
                int dims,
                TAY_SPACE_DESTROY_FUNC destroy,
                TAY_SPACE_ADD_FUNC add,
                TAY_SPACE_SEE_FUNC see,
                TAY_SPACE_ACT_FUNC act,
                TAY_SPACE_UPDATE_FUNC update,
                TAY_SPACE_SIM_START_FUNC on_simulation_start,
                TAY_SPACE_SIM_END_FUNC on_simulation_end) {
    space->storage = storage;
    space->dims = dims;
    space->destroy = destroy;
    space->add = add;
    space->see = see;
    space->act = act;
    space->update = update;
}

void tay_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float *radii, int dims, TayThreadContext *thread_context) {
    for (TayAgentTag *a = seer_agents; a; a = a->next) {
        float *pa = TAY_AGENT_POSITION(a);

        for (TayAgentTag *b = seen_agents; b; b = b->next) {
            float *pb = TAY_AGENT_POSITION(b);

            if (a == b) /* this can be removed for cases where beg_a != beg_b */
                continue;

#if TAY_INSTRUMENT
            ++thread_context->broad_see_phase;
#endif

            for (int k = 0; k < dims; ++k) {
                float d = pa[k] - pb[k];
                if (d < -radii[k] || d > radii[k])
                    goto OUTSIDE_RADII;
            }

#if TAY_INSTRUMENT
            ++thread_context->narrow_see_phase;
#endif

            func(TAY_AGENT_DATA(a), TAY_AGENT_DATA(b), thread_context->context);

            OUTSIDE_RADII:;
        }
    }
}
