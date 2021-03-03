#include "space.h"
#include "thread.h"
#include <stdlib.h>
#include <float.h>
#include <string.h>
#include <assert.h>


void space_init(Space *space) {
    space->type = ST_NONE;
    space->requested_type = ST_CPU_SIMPLE;
    space->dims = 3;
    space->radii = (float4){ 10.0f, 10.0f, 10.0f, 10.0f };
    space->depth_correction = 0;
    space->shared = 0;
    space->shared_size = 0;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        space->first[i] = 0;
        space->counts[i] = 0;
    }
    // space_gpu_shared_init(&space->gpu_shared);
}

void space_release(Space *space) {
    free(space->shared);
    // space_gpu_shared_release(&space->gpu_shared);
}

// void *space_get_temp_arena(Space *space, int size) {
//     assert(size <= TAY_CPU_SHARED_TEMP_ARENA_SIZE);
//     return space->cpu_shared.temp_arena;
// }

// void *space_get_cell_arena(Space *space, int size, int zero) {
//     assert(size <= TAY_CPU_SHARED_CELL_ARENA_SIZE);
//     if (zero)
//         memset(space->cpu_shared.cell_arena, 0, size);
//     return space->cpu_shared.cell_arena;
// }

// int space_get_thread_mem_size() {
//     int rem = TAY_CPU_SHARED_THREAD_ARENA_SIZE % runner.count;
//     return (TAY_CPU_SHARED_THREAD_ARENA_SIZE - rem) / runner.count;
// }

// void *space_get_thread_mem(Space *space, int thread_i) {
//     int size = space_get_thread_mem_size();
//     return space->cpu_shared.thread_arena + size * thread_i;
// }

void space_add_agent(Space *space, TayAgentTag *agent, int group) {
    agent->next = space->first[group];
    space->first[group] = agent;
    box_update(&space->box, float4_agent_position(agent), space->dims);
    ++space->counts[group];
}

void space_on_simulation_start(Space *space) {
    // space_gpu_on_simulation_start(state); /* compose/build program, create all shared kernels and buffers */
    space->type = ST_NONE;
    space->requested_type = ST_CPU_SIMPLE;
}

void space_on_simulation_end(Space *space) {
    // space_gpu_on_simulation_end(state); /* release all shared kernels and buffers */
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

#if TAY_TELEMETRY
            ++thread_context->broad_see_phase;
#endif

            for (int i = 0; i < dims; ++i) {
                float d = seer_p.arr[i] - seen_p.arr[i];
                if (d < -radii.arr[i] || d > radii.arr[i])
                    goto SKIP_SEE;
            }

#if TAY_TELEMETRY
            ++thread_context->narrow_see_phase;
#endif

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_single_seer_see(TayAgentTag *seer_agent, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    float4 seer_p = float4_agent_position(seer_agent);

    for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {
        float4 seen_p = float4_agent_position(seen_agent);

        if (seer_agent == seen_agent)
            continue;

#if TAY_TELEMETRY
        ++thread_context->broad_see_phase;
#endif

        for (int i = 0; i < dims; ++i) {
            float d = seer_p.arr[i] - seen_p.arr[i];
            if (d < -radii.arr[i] || d > radii.arr[i])
                goto SKIP_SEE;
        }

#if TAY_TELEMETRY
        ++thread_context->narrow_see_phase;
#endif

        func(seer_agent, seen_agent, thread_context->context);

        SKIP_SEE:;
    }
}

int group_tag_to_index(TayGroup *group, TayAgentTag *tag) {
    return (tag != 0) ? (int)((char *)tag - (char *)group->storage) / group->agent_size : TAY_GPU_NULL_INDEX;
}

int space_agent_count_to_bucket_index(int count) {
    /* both these consts must be within [0, TAY_MAX_BUCKETS> */
    const int min_pow = 5;
    const int max_pow = 20;
    int pow = min_pow;
    while (pow < max_pow && (1 << pow) < count)
        ++pow;
    return max_pow - pow;
}
