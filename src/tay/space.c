#include "space.h"
#include "thread.h"
#include <float.h>
#include <assert.h>


void space_init(Space *space, int dims, float4 radii, int depth_correction, SpaceType init_type) {
    space->type = init_type;
    space->dims = dims;
    space->radii = radii;
    space->depth_correction = depth_correction;
    space->shared_in_use = 0;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        space->first[i] = 0;
        space->counts[i] = 0;
    }
    space_gpu_shared_init(&space->gpu_shared);
}

void space_release(Space *space) {
    space_gpu_shared_release(&space->gpu_shared);
}

void space_add_agent(Space *space, TayAgentTag *agent, int group) {
    agent->next = space->first[group];
    space->first[group] = agent;
    box_update(&space->box, TAY_AGENT_POSITION(agent), space->dims);
    ++space->counts[group];
}

void *space_get_shared_mem(Space *space, int size) {
    assert(space->shared_in_use == 0);
    assert(size <= TAY_SPACE_SHARED_SIZE);
    space->shared_in_use = 1;
    return space->shared;
}

void space_release_shared_mem(Space *space) {
    space->shared_in_use = 0;
}

void space_on_simulation_start(TayState *state) {
    space_gpu_on_simulation_start(state);
    if (state->space.type == ST_GPU_SIMPLE)
        gpu_simple_on_simulation_start(state);
    else if (state->space.type == ST_GPU_TREE)
        assert(0); /* not implemented */
}

void space_run(TayState *state, int steps) {

    if (state->space.type & ST_GPU)
        space_gpu_shared_on_run_start(state);

    for (int step_i = 0; step_i < steps; ++step_i) {

        // TODO: determine space type when adaptive

        if (state->space.type == ST_CPU_SIMPLE)
            cpu_simple_step(state);
        else if (state->space.type == ST_CPU_TREE)
            cpu_tree_step(state);
        else
            assert(0); /* unhandled space type */

        if (state->space.type & ST_GPU)
            space_gpu_shared_fetch_new_positions(state);
    }

    if (state->space.type & ST_GPU)
        space_gpu_shared_on_run_end(state);
}

void space_on_simulation_end(TayState *state) {
    if (state->space.type == ST_GPU_TREE)
        assert(0); /* not implemented */
    space_gpu_on_simulation_end(state);
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

void space_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (TayAgentTag *seer_agent = seer_agents; seer_agent; seer_agent = seer_agent->next) {
        float4 seer_p = TAY_AGENT_POSITION(seer_agent);

        for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {
            float4 seen_p = TAY_AGENT_POSITION(seen_agent);

            if (seer_agent == seen_agent) /* this can be removed for cases where beg_a != beg_b */
                continue;

#if TAY_INSTRUMENT
            ++thread_context->broad_see_phase;
#endif

            for (int i = 0; i < dims; ++i) {
                float d = seer_p.arr[i] - seen_p.arr[i];
                if (d < -radii.arr[i] || d > radii.arr[i])
                    goto SKIP_SEE;
            }

#if TAY_INSTRUMENT
            ++thread_context->narrow_see_phase;
#endif

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

int group_tag_to_index(TayGroup *group, TayAgentTag *tag) {
    return (tag != 0) ? (int)((char *)tag - (char *)group->storage) / group->agent_size : TAY_GPU_NULL_INDEX;
}
