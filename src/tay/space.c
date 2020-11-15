#include "state.h"
#include "space_impl.h"
#include <float.h>
#include <assert.h>


void space_init(Space *space, int dims, SpaceType init_type) {
    space->type = init_type;
    space->dims = dims;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        space->first[i] = 0;
        space->counts[i] = 0;
    }
}

void space_add_agent(Space *space, TayAgentTag *agent, int group) {
    agent->next = space->first[group];
    space->first[group] = agent;
    box_update(&space->box, TAY_AGENT_POSITION(agent), space->dims);
    ++space->counts[group];
}

void space_on_simulation_start(TayState *state) {
    // ...
}

void space_run(TayState *state, int steps) {

    // on run start

    for (int step_i = 0; step_i < steps; ++step_i) {

        // TODO: determine space type

        if (state->_space.type == ST_CPU_SIMPLE)
            cpu_simple_step(state);
        else if (state->_space.type == ST_CPU_TREE)
            /* cpu_tree_step(state) */;
        else
            assert(0); /* unhandled space type */
    }

    // on run end
}

void space_on_simulation_end(TayState *state) {
    // ...
}

void box_update(Box *box, float4 p, int dims) {
    for (int i = 0; i < dims; ++i) {
        if (p.arr[i] < box->min.arr[i])
            box->min.arr[i] = p.arr[i];
        if (p.arr[i] > box->max.arr[i])
            box->max.arr[i] = p.arr[i];
    }
}

void box_reset(Box *box, int dims) {
    for (int i = 0; i < dims; ++i) {
        box->min.arr[i] = FLT_MAX;
        box->max.arr[i] = -FLT_MAX;
    }
}

void space_return_agents(Space *space, int group_i, TayAgentTag *tag) {
    if (tag == 0)
        return;
    TayAgentTag *last = tag;
    while (1) {
        box_update(&space->box, TAY_AGENT_POSITION(last), space->dims);
        ++space->counts[group_i];
        if (last->next)
            last = last->next;
        else
            break;
    }
    last->next = space->first[group_i];
    space->first[group_i] = tag;
}
