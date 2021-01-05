#include "space.h"
#include "thread.h"
#include <float.h>
#include <string.h>
#include <assert.h>


void space_init(Space *space, int dims, float4 radii) {
    space->type = ST_NONE;
    space->dims = dims;
    space->radii = radii;
    space->depth_correction = 0;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        space->first[i] = 0;
        space->counts[i] = 0;
    }
    space_gpu_shared_init(&space->gpu_shared);
}

void space_release(Space *space) {
    space_gpu_shared_release(&space->gpu_shared);
}

void *space_get_temp_arena(Space *space, int size) {
    assert(size <= TAY_CPU_SHARED_TEMP_ARENA_SIZE);
    return space->cpu_shared.temp_arena;
}

void *space_get_cell_arena(Space *space, int size, int zero) {
    assert(size <= TAY_CPU_SHARED_CELL_ARENA_SIZE);
    if (zero)
        memset(space->cpu_shared.cell_arena, 0, size);
    return space->cpu_shared.cell_arena;
}

int space_get_thread_mem_size() {
    int rem = TAY_CPU_SHARED_THREAD_ARENA_SIZE % runner.count;
    return (TAY_CPU_SHARED_THREAD_ARENA_SIZE - rem) / runner.count;
}

void *space_get_thread_mem(Space *space, int thread_i) {
    int size = space_get_thread_mem_size();
    return space->cpu_shared.thread_arena + size * thread_i;
}

void space_add_agent(Space *space, TayAgentTag *agent, int group) {
    agent->next = space->first[group];
    space->first[group] = agent;
    box_update(&space->box, float4_agent_position(agent), space->dims);
    ++space->counts[group];
}

void space_on_simulation_start(TayState *state) {
    space_gpu_on_simulation_start(state);   /* compose/build program, create all shared kernels and buffers */
    state->space.type = ST_NONE;
}

void space_on_simulation_end(TayState *state) {
    space_gpu_on_simulation_end(state);     /* release all shared kernels and buffers */
}

void space_run(TayState *state, int steps, SpaceType space_type, int depth_correction) {
    Space *space = &state->space;
    assert(space_type & ST_FINAL);

    int cycling = space_type == ST_CYCLE_ALL;

    for (int step_i = 0; step_i < steps; ++step_i) {
        SpaceType old_type = space->type;

        if (cycling) {
            if (space_type == ST_CPU_SIMPLE)
                space_type = ST_CPU_TREE;
            else if (space_type == ST_CPU_TREE)
                space_type = ST_CPU_GRID;
            else if (space_type == ST_CPU_GRID)
                space_type = ST_GPU_SIMPLE_DIRECT;
            else if (space_type == ST_GPU_SIMPLE_DIRECT)
                space_type = ST_GPU_SIMPLE_INDIRECT;
            else if (space_type == ST_GPU_SIMPLE_INDIRECT)
                space_type = ST_GPU_TREE;
            else
                space_type = ST_CPU_SIMPLE;
        }

        SpaceType new_type = space_type;
        // TODO: determine space type when adaptive, for now we just fix one of the specific types
        space->type = new_type;

        // TODO: determine depth correction when adaptive, for now we just take the fixed one from the argument
        space->depth_correction = depth_correction;

        if (new_type & ST_GPU) {
            if (old_type & ST_CPU || old_type == ST_NONE)   /* switching from cpu to gpu or first gpu step in the simulation */
                space_gpu_push_agents(state);
            else {                                          /* not switching from gpu to cpu and not first step in the simulation */
                if (new_type == ST_GPU_TREE)
                    space_gpu_fetch_agent_positions(state);
            }
            if ((new_type & ST_GPU_SIMPLE) && (old_type & ST_GPU_SIMPLE) == 0)
                gpu_simple_fix_gpu_pointers(state);
        }
        else if (new_type & ST_CPU) {
            if (old_type & ST_GPU)                          /* switching from gpu to cpu */
                space_gpu_fetch_agents(state);
            if (new_type == ST_CPU_GRID && old_type != ST_CPU_GRID) /* prepare cpu grid */
                cpu_grid_prepare(state);
        }

        if (space->type == ST_CPU_SIMPLE)
            cpu_simple_step(state);
        else if (space->type == ST_CPU_TREE)
            cpu_tree_step(state);
        else if (space->type == ST_CPU_GRID)
            cpu_grid_step(state);
        else if (space->type & ST_GPU_SIMPLE)
            gpu_simple_step(state, space->type == ST_GPU_SIMPLE_DIRECT);
        else if (space->type == ST_GPU_TREE)
            gpu_tree_step(state);
        else
            assert(0); /* unhandled space type */
    }

    if (space->type & ST_GPU)
        space_gpu_fetch_agents(state);
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
        box_update(&space->box, float4_agent_position(last), space->dims);
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
        float4 seer_p = float4_agent_position(seer_agent);

        for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {
            float4 seen_p = float4_agent_position(seen_agent);

            if (seer_agent == seen_agent)
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

void space_see_single_seer(TayAgentTag *seer_agent, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    float4 seer_p = float4_agent_position(seer_agent);

    for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {
        float4 seen_p = float4_agent_position(seen_agent);

        if (seer_agent == seen_agent)
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

int group_tag_to_index(TayGroup *group, TayAgentTag *tag) {
    return (tag != 0) ? (int)((char *)tag - (char *)group->storage) / group->agent_size : TAY_GPU_NULL_INDEX;
}
