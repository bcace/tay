#ifndef tay_state_h
#define tay_state_h

#include "space.h"


typedef void (*TAY_SEE_FUNC)(void *, void *, void *);
typedef void (*TAY_ACT_FUNC)(void *, void *);

typedef struct TayGroup {
    void *storage;              /* agents storage */
    struct TayAgentTag *first;  /* single linked list of available agents from storage */
    int agent_size;             /* agent size in bytes */
    int capacity;               /* max. number of agents */
    int is_point;
} TayGroup;

typedef enum TayPassType {
    TAY_PASS_SEE,
    TAY_PASS_ACT,
} TayPassType;

typedef struct TayPass {
    TayPassType type;
    union {
        int act_group;
        int seer_group;
    };
    int seen_group;
    union {
        TAY_SEE_FUNC see;
        TAY_ACT_FUNC act;
    };
    const char *func_name;
    float4 radii;
    void *context;
    int context_size;
} TayPass;

typedef enum TayStateStatus {
    TAY_STATE_STATUS_IDLE,
    TAY_STATE_STATUS_RUNNING,
} TayStateStatus;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    int passes_count;
    Space space;
    TayStateStatus running;
    const char *source;
} TayState;

int group_tag_to_index(TayGroup *group, TayAgentTag *tag);

// TODO: move this to space module
/* Shared CPU version of see between linked lists of agents. */
void tay_see(struct TayAgentTag *seer_agents, struct TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

#endif
