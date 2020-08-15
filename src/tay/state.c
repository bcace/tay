#include "state.h"
#include "tay.h"
#include <stdlib.h>
#include <assert.h>


TayState *tay_create_state(TaySpaceType space_type, int space_dimensions, float *space_diameters) {
    TayState *s = calloc(1, sizeof(TayState));
    if (space_type == TAY_SPACE_SIMPLE)
        space_simple_init(&s->space);
    else if (space_type == TAY_SPACE_TREE)
        tree_init(&s->space, space_dimensions, space_diameters);
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

void tay_add_perception(TayState *state, int group1, int group2, void (*func)(void *, void *, void *)) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_PERCEPTION;
    p->perception = func;
    p->group1 = group1;
    p->group2 = group2;
}

void tay_add_action(TayState *state, int group, void (*func)(void *, void *)) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_ACTION;
    p->action = func;
    p->group1 = group;
}

void tay_add_post(struct TayState *state, void (*func)(void *)) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->type = TAY_PASS_POST;
    p->post = func;
}

void *tay_alloc_agent(TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    assert(g->first != 0);
    TayAgent *a = g->first;
    g->first = g->first->next;
    state->space.add(&state->space, a, group);
    return a;
}

void *tay_get_storage(struct TayState *state, int group) {
    assert(group >= 0 && group < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group;
    return g->storage;
}

void tay_run(TayState *state, int steps, void *context) {
    for (int i = 0; i < steps; ++i) {
        state->space.update(&state->space);
        for (int j = 0; j < state->passes_count; ++j) {
            TayPass *p = state->passes + j;
            if (p->type == TAY_PASS_PERCEPTION)
                state->space.perception(&state->space, p->group1, p->group2, p->perception, context);
            else if (p->type == TAY_PASS_ACTION)
                state->space.action(&state->space, p->group1, p->action, context);
            else if (p->type == TAY_PASS_POST)
                state->space.post(&state->space, p->post, context);
            else
                assert(0); /* not implemented */
        }
    }
}

void tay_iter_agents(struct TayState *state, int group, void (*func)(void *, void *), void *context) {
    state->space.iter(&state->space, group, func, context);
}

void space_init(TaySpace *space,
                void *storage,
                void (*destroy)(TaySpace *space),
                void (*add)(TaySpace *space, TayAgent *agent, int group),
                void (*perception)(TaySpace *space, int group1, int group2, void (*func)(void *, void *, void *), void *context),
                void (*action)(TaySpace *space, int group, void (*func)(void *, void *), void *context),
                void (*post)(TaySpace *space, void (*func)(void *), void *context),
                void (*iter)(TaySpace *space, int group, void (*func)(void *, void *), void *context),
                void (*update)(TaySpace *space)) {
    space->storage = storage;
    space->destroy = destroy;
    space->add = add;
    space->perception = perception;
    space->action = action;
    space->post = post;
    space->iter = iter;
    space->update = update;
}
