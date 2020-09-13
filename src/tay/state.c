#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>


TayState *tay_create_state(TaySpaceType space_type, int space_dimensions, float *space_radii, int max_depth_correction) {
    TayState *s = calloc(1, sizeof(TayState));
    if (space_type == TAY_SPACE_SIMPLE)
        space_simple_init(&s->space, space_dimensions);
    else if (space_type == TAY_SPACE_TREE)
        tree_init(&s->space, space_dimensions, space_radii, max_depth_correction);
    else
        assert(0); /* not implemented */
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
    assert(agent_size >= sizeof(TayAgent));
    assert(agent_capacity != 0);
    int index = 0;
    for (; index < TAY_MAX_GROUPS; ++index)
        if (state->groups[index].storage == 0)
            break;
    assert(index < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + index;
    g->storage = calloc(agent_capacity, agent_size);
    g->size = agent_size;
    g->capacity = agent_capacity;
    g->first = g->storage;
    TayAgent *prev = g->first;
    for (int i = 0; i < agent_capacity - 1; ++i) {
        TayAgent *next = (TayAgent *)((char *)prev + agent_size);
        prev->next = next;
        prev = next;
    }
    prev->next = 0;
    return index;
}

void tay_add_see(TayState *state, int seer_group, int seen_group, void (*func)(void *, void *, struct TayThreadContext *), float *radii) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_SEE;
    p->see = func;
    p->seer_group = seer_group;
    p->seen_group = seen_group;
    for (int i = 0; i < TAY_MAX_DIMENSIONS; ++i)
        p->radii[i] = radii[i];
}

void tay_add_act(TayState *state, int act_group, void (*func)(void *, struct TayThreadContext *)) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_ACT;
    p->act = func;
    p->act_group = act_group;
}

void tay_add_post(struct TayState *state, void (*func)(void *)) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_POST;
    p->post = func;
}

void *tay_get_new_agent(TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    assert(g->first != 0);
    return g->first;
}

void tay_add_new_agent(TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    assert(g->first != 0);
    TayAgent *a = g->first;
    g->first = a->next;
    state->space.add(&state->space, a, group);
}

void *tay_get_storage(struct TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    return g->storage;
}

void tay_run(TayState *state, int steps, void *context) {
    struct timespec beg, end;
    timespec_get(&beg, TIME_UTC);

    tay_runner_reset_stats();

    for (int i = 0; i < steps; ++i) {
        state->space.update(&state->space);
        for (int j = 0; j < state->passes_count; ++j) {
            TayPass *p = state->passes + j;
            if (p->type == TAY_PASS_SEE)
                state->space.see(&state->space, p, context);
            else if (p->type == TAY_PASS_ACT)
                state->space.act(&state->space, p, context);
            else if (p->type == TAY_PASS_POST)
                state->space.post(&state->space, p->post, context);
            else
                assert(0); /* not implemented */
        }
    }

    timespec_get(&end, TIME_UTC);
    double t = (end.tv_sec - beg.tv_sec) + ((long long)end.tv_nsec - (long long)beg.tv_nsec) * 1.0e-9;
    double fps = steps / t;

    tay_runner_report_stats();
    printf("run time: %g sec, %g fps\n\n", t, fps);
}

void tay_iter_agents(struct TayState *state, int group, void (*func)(void *, void *), void *context) {
    state->space.iter(&state->space, group, func, context);
}

void space_init(TaySpace *space,
                void *storage,
                int dims,
                void (*destroy)(TaySpace *space),
                void (*add)(TaySpace *space, TayAgent *agent, int group),
                void (*see)(TaySpace *space, TayPass *pass, void *context),
                void (*act)(TaySpace *space, TayPass *pass, void *context),
                void (*post)(TaySpace *space, void (*func)(void *), void *context),
                void (*iter)(TaySpace *space, int group, void (*func)(void *, void *), void *context),
                void (*update)(TaySpace *space)) {
    space->storage = storage;
    space->dims = dims;
    space->destroy = destroy;
    space->add = add;
    space->see = see;
    space->act = act;
    space->post = post;
    space->iter = iter;
    space->update = update;
}

void tay_see(TayAgent *seer_agents, TayAgent *seen_agents, TAY_SEE_FUNC func, float *radii, int dims, TayThreadContext *thread_context) {
    for (TayAgent *a = seer_agents; a; a = a->next) {
        float *pa = TAY_AGENT_POSITION(a);

        for (TayAgent *b = seen_agents; b; b = b->next) {
            float *pb = TAY_AGENT_POSITION(b);

            if (a == b) /* this can be removed for cases where beg_a != beg_b */
                continue;

            ++thread_context->broad_see_phase;

            for (int k = 0; k < dims; ++k) {
                float d = pa[k] - pb[k];
                if (d < -radii[k] || d > radii[k])
                    goto OUTSIDE_RADII;
            }

            ++thread_context->narrow_see_phase;

            func(a, b, thread_context->context);

            OUTSIDE_RADII:;
        }
    }
}
