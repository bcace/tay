#include "state.h"
#include "tay.h"
#include <stdlib.h>
#include <assert.h>


TayState *tay_create_state(TaySpaceType space_type) {
    TayState *s = calloc(1, sizeof(TayState));
    if (space_type == TAY_SPACE_PLAIN)
        space_plain_init(&s->space);
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
    p->perception = func;
    p->group1 = group1;
    p->group2 = group2;
}

void tay_add_action(TayState *state, int group, void (*func)(void *, void *)) {
    assert(state->passes_count < TAY_MAX_PASSES);
    TayPass *p = state->passes + state->passes_count++;
    p->action = func;
    p->group1 = group;
}

void *tay_alloc_agent(TayState *state, int group_index) {
    assert(group_index >= 0 && group_index < TAY_MAX_GROUPS);
    TayGroup *g = state->groups + group_index;
    assert(g->first != 0);
    TayAgent *a = g->first;
    g->first = g->first->next;
    state->space.add(&state->space, a, group_index);
    return a;
}

void tay_run(TayState *state, int steps, void *context) {
    for (int i = 0; i < steps; ++i) {
        for (int j = 0; j < state->passes_count; ++j) {
            TayPass *p = state->passes + j;
            if (p->perception)
                state->space.perception(&state->space, p->group1, p->group2, p->perception, context);
            else if (p->action)
                state->space.action(&state->space, p->group1, p->action, context);
            else
                assert(0); /* not implemented */
        }
    }
}
