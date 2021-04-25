#include "space.h"
#include "thread.h"
#include <stdlib.h>
#include <float.h>
#include <string.h>
#include <assert.h>


void box_update_from_agent(Box *box, char *agent, int dims, int is_point) {
    float4 min = float4_agent_min(agent);
    if (is_point) {
        for (int i = 0; i < dims; ++i) {
            if (min.arr[i] < box->min.arr[i])
                box->min.arr[i] = min.arr[i];
            if (min.arr[i] > box->max.arr[i])
                box->max.arr[i] = min.arr[i];
        }
    }
    else {
        float4 max = float4_agent_max(agent);
        for (int i = 0; i < dims; ++i) {
            if (min.arr[i] < box->min.arr[i])
                box->min.arr[i] = min.arr[i];
            if (max.arr[i] > box->max.arr[i])
                box->max.arr[i] = max.arr[i];
        }
    }
}

void box_reset(Box *box, int dims) {
    for (int i = 0; i < dims; ++i) {
        box->min.arr[i] = FLT_MAX;
        box->max.arr[i] = -FLT_MAX;
    }
}

void space_return_agents(Space *space, TayAgentTag *tag, int is_point) {
    if (tag == 0)
        return;
    TayAgentTag *last = tag;
    while (1) {
        box_update_from_agent(&space->box, (char *)last, space->dims, is_point);
        ++space->count;
        if (last->next)
            last = last->next;
        else
            break;
    }
    last->next = space->first;
    space->first = tag;
}

void space_update_box(TayGroup *group) {
    Space *space = &group->space;
    box_reset(&space->box, space->dims);
    for (unsigned i = 0; i < space->count; ++i) {
        char *agent = group->storage + group->agent_size * i;
        box_update_from_agent(&space->box, agent, space->dims, group->is_point);
    }
}

#if TAY_TELEMETRY
#define _BROAD_PHASE_COUNT ++thread_context->broad_see_phase;
#else
#define _BROAD_PHASE_COUNT ;
#endif

#if TAY_TELEMETRY
#define _NARROW_PHASE_COUNT ++thread_context->narrow_see_phase;
#else
#define _NARROW_PHASE_COUNT ;
#endif

#define _POINT_POINT_NARROW_PHASE_TEST { \
    for (int i = 0; i < dims; ++i) { \
        float d = seer_p.arr[i] - seen_p.arr[i]; \
        if (d < -radii.arr[i] || d > radii.arr[i]) \
            goto SKIP_SEE; \
    } \
}

#define _NONPOINT_POINT_NARROW_PHASE_TEST { \
    for (int i = 0; i < dims; ++i) { \
        float d; \
        d = seer_min.arr[i] - seen_p.arr[i]; \
        if (d > radii.arr[i]) \
            goto SKIP_SEE; \
        d = seen_p.arr[i] - seer_max.arr[i]; \
        if (d > radii.arr[i]) \
            goto SKIP_SEE; \
    } \
}

#define _POINT_NONPOINT_NARROW_PHASE_TEST { \
    for (int i = 0; i < dims; ++i) { \
        float d; \
        d = seer_p.arr[i] - seen_max.arr[i]; \
        if (d > radii.arr[i]) \
            goto SKIP_SEE; \
        d = seen_min.arr[i] - seer_p.arr[i]; \
        if (d > radii.arr[i]) \
            goto SKIP_SEE; \
    } \
}

#define _NONPOINT_NONPOINT_NARROW_PHASE_TEST { \
    for (int i = 0; i < dims; ++i) { \
        float d; \
        d = seer_min.arr[i] - seen_max.arr[i]; \
        if (d > radii.arr[i]) \
            goto SKIP_SEE; \
        d = seen_min.arr[i] - seer_max.arr[i]; \
        if (d > radii.arr[i]) \
            goto SKIP_SEE; \
    } \
}

void space_see_point_point(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (TayAgentTag *seer_agent = seer_agents; seer_agent; seer_agent = seer_agent->next) {
        float4 seer_p = float4_agent_position(seer_agent);

        for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {

            if (seer_agent == seen_agent)
                continue;

            float4 seen_p = float4_agent_position(seen_agent);

            _BROAD_PHASE_COUNT
            _POINT_POINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

/* should only be applied on see between the same type that specified that agent should not see itself */
void space_see_point_point_new(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (unsigned seer_i = seer_slice.beg; seer_i < seer_slice.end; ++seer_i) {
        TayAgentTag *seer_agent = (TayAgentTag *)(seer_slice.agents + seer_slice.size * seer_i);
        float4 seer_p = float4_agent_position(seer_agent);

        for (unsigned seen_i = seen_slice.beg; seen_i < seen_slice.end; ++seen_i) {

            if (seer_i == seen_i)
                continue;

            TayAgentTag *seen_agent = (TayAgentTag *)(seen_slice.agents + seen_slice.size * seen_i);
            float4 seen_p = float4_agent_position(seen_agent);
            
            _BROAD_PHASE_COUNT
            _POINT_POINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_point_point_self_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (TayAgentTag *seer_agent = seer_agents; seer_agent; seer_agent = seer_agent->next) {
        float4 seer_p = float4_agent_position(seer_agent);

        for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {
            float4 seen_p = float4_agent_position(seen_agent);

            _BROAD_PHASE_COUNT
            _POINT_POINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_point_point_self_see_new(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (unsigned seer_i = seer_slice.beg; seer_i < seer_slice.end; ++seer_i) {
        TayAgentTag *seer_agent = (TayAgentTag *)(seer_slice.agents + seer_slice.size * seer_i);
        float4 seer_p = float4_agent_position(seer_agent);

        for (unsigned seen_i = seen_slice.beg; seen_i < seen_slice.end; ++seen_i) {
            TayAgentTag *seen_agent = (TayAgentTag *)(seen_slice.agents + seen_slice.size * seen_i);
            float4 seen_p = float4_agent_position(seen_agent);
            
            _BROAD_PHASE_COUNT
            _POINT_POINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_nonpoint_point(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (TayAgentTag *seer_agent = seer_agents; seer_agent; seer_agent = seer_agent->next) {
        float4 seer_min = float4_agent_min(seer_agent);
        float4 seer_max = float4_agent_max(seer_agent);

        for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {

            float4 seen_p = float4_agent_position(seen_agent);

            _BROAD_PHASE_COUNT
            _NONPOINT_POINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_nonpoint_point_new(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (unsigned seer_i = seer_slice.beg; seer_i < seer_slice.end; ++seer_i) {
        TayAgentTag *seer_agent = (TayAgentTag *)(seer_slice.agents + seer_slice.size * seer_i);
        float4 seer_min = float4_agent_min(seer_agent);
        float4 seer_max = float4_agent_max(seer_agent);

        for (unsigned seen_i = seen_slice.beg; seen_i < seen_slice.end; ++seen_i) {
            TayAgentTag *seen_agent = (TayAgentTag *)(seen_slice.agents + seen_slice.size * seen_i);
            float4 seen_p = float4_agent_position(seen_agent);

            _BROAD_PHASE_COUNT
            _NONPOINT_POINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}


void space_see_point_nonpoint(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (TayAgentTag *seer_agent = seer_agents; seer_agent; seer_agent = seer_agent->next) {
        float4 seer_p = float4_agent_position(seer_agent);

        for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {
            float4 seen_min = float4_agent_min(seen_agent);
            float4 seen_max = float4_agent_max(seen_agent);

            _BROAD_PHASE_COUNT
            _POINT_NONPOINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_point_nonpoint_new(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (unsigned seer_i = seer_slice.beg; seer_i < seer_slice.end; ++seer_i) {
        TayAgentTag *seer_agent = (TayAgentTag *)(seer_slice.agents + seer_slice.size * seer_i);
        float4 seer_p = float4_agent_position(seer_agent);

        for (unsigned seen_i = seen_slice.beg; seen_i < seen_slice.end; ++seen_i) {
            TayAgentTag *seen_agent = (TayAgentTag *)(seen_slice.agents + seen_slice.size * seen_i);
            float4 seen_min = float4_agent_min(seen_agent);
            float4 seen_max = float4_agent_max(seen_agent);

            _BROAD_PHASE_COUNT
            _POINT_NONPOINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_nonpoint_nonpoint(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (TayAgentTag *seer_agent = seer_agents; seer_agent; seer_agent = seer_agent->next) {
        float4 seer_min = float4_agent_min(seer_agent);
        float4 seer_max = float4_agent_max(seer_agent);

        for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {

            if (seer_agent == seen_agent)
                continue;

            float4 seen_min = float4_agent_min(seen_agent);
            float4 seen_max = float4_agent_max(seen_agent);

            _BROAD_PHASE_COUNT
            _NONPOINT_NONPOINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_nonpoint_nonpoint_new(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (unsigned seer_i = seer_slice.beg; seer_i < seer_slice.end; ++seer_i) {
        TayAgentTag *seer_agent = (TayAgentTag *)(seer_slice.agents + seer_slice.size * seer_i);
        float4 seer_min = float4_agent_min(seer_agent);
        float4 seer_max = float4_agent_max(seer_agent);

        for (unsigned seen_i = seen_slice.beg; seen_i < seen_slice.end; ++seen_i) {

            if (seer_i == seen_i)
                continue;

            TayAgentTag *seen_agent = (TayAgentTag *)(seen_slice.agents + seen_slice.size * seen_i);
            float4 seen_min = float4_agent_min(seen_agent);
            float4 seen_max = float4_agent_max(seen_agent);

            _BROAD_PHASE_COUNT
            _NONPOINT_NONPOINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_nonpoint_nonpoint_self_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (TayAgentTag *seer_agent = seer_agents; seer_agent; seer_agent = seer_agent->next) {
        float4 seer_min = float4_agent_min(seer_agent);
        float4 seer_max = float4_agent_max(seer_agent);

        for (TayAgentTag *seen_agent = seen_agents; seen_agent; seen_agent = seen_agent->next) {
            float4 seen_min = float4_agent_min(seen_agent);
            float4 seen_max = float4_agent_max(seen_agent);

            _BROAD_PHASE_COUNT
            _NONPOINT_NONPOINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
}

void space_see_nonpoint_nonpoint_self_see_new(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, TayThreadContext *thread_context) {
    for (unsigned seer_i = seer_slice.beg; seer_i < seer_slice.end; ++seer_i) {
        TayAgentTag *seer_agent = (TayAgentTag *)(seer_slice.agents + seer_slice.size * seer_i);
        float4 seer_min = float4_agent_min(seer_agent);
        float4 seer_max = float4_agent_max(seer_agent);

        for (unsigned seen_i = seen_slice.beg; seen_i < seen_slice.end; ++seen_i) {
            TayAgentTag *seen_agent = (TayAgentTag *)(seen_slice.agents + seen_slice.size * seen_i);
            float4 seen_min = float4_agent_min(seen_agent);
            float4 seen_max = float4_agent_max(seen_agent);

            _BROAD_PHASE_COUNT
            _NONPOINT_NONPOINT_NARROW_PHASE_TEST
            _NARROW_PHASE_COUNT

            func(seer_agent, seen_agent, thread_context->context);

            SKIP_SEE:;
        }
    }
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
