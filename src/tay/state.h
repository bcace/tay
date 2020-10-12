#ifndef tay_state_h
#define tay_state_h

#define TAY_MAX_GROUPS      8
#define TAY_MAX_PASSES      32
#define TAY_MAX_DIMENSIONS  8
#define TAY_INSTRUMENT      0
#define TAY_AGENT_TAG_SIZE  8

#define TAY_AGENT_POSITION(__agent_tag__) ((float *)(__agent_tag__ + 1))
#define TAY_AGENT_DATA(__agent_tag__) ((void *)(__agent_tag__ + 1))


typedef void (*TAY_SEE_FUNC)(void *, void *, void *);
typedef void (*TAY_ACT_FUNC)(void *, void *);

typedef void (*TAY_SPACE_DESTROY_FUNC)(struct TaySpaceContainer *space);
typedef void (*TAY_SPACE_ADD_FUNC)(struct TaySpaceContainer *space, struct TayAgentTag *agent, int group, int index);
typedef void (*TAY_SPACE_SEE_FUNC)(struct TaySpaceContainer *space, struct TayPass *pass);
typedef void (*TAY_SPACE_ACT_FUNC)(struct TaySpaceContainer *space, struct TayPass *pass);
typedef void (*TAY_SPACE_UPDATE_FUNC)(struct TaySpaceContainer *space);
typedef void (*TAY_SPACE_SIM_START_FUNC)(struct TaySpaceContainer *space, struct TayState *state);
typedef void (*TAY_SPACE_SIM_END_FUNC)(struct TaySpaceContainer *space);

typedef struct TayAgentTag {
    struct TayAgentTag *next;
} TayAgentTag;

// TODO: turn storage into char *
typedef struct TayGroup {
    void *storage;              /* agents storage */
    struct TayAgentTag *first;  /* single linked list of available agents from storage */
    int agent_size;             /* agent size in bytes */
    int agent_size_with_tag;
    int capacity;               /* max. number of agents */
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
        void (*post)(void *);
    };
    float radii[TAY_MAX_DIMENSIONS];
    void *context;
    int context_size;
} TayPass;

typedef struct TaySpaceContainer {
    void *storage;
    int dims;
    TAY_SPACE_DESTROY_FUNC destroy;
    TAY_SPACE_ADD_FUNC add;
    TAY_SPACE_SEE_FUNC see;
    TAY_SPACE_ACT_FUNC act;
    TAY_SPACE_UPDATE_FUNC update;
    TAY_SPACE_SIM_START_FUNC on_simulation_start;
    TAY_SPACE_SIM_END_FUNC on_simulation_end;
} TaySpaceContainer;

typedef enum TayStateStatus {
    TAY_STATE_STATUS_IDLE,
    TAY_STATE_STATUS_RUNNING,
} TayStateStatus;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    int passes_count;
    TaySpaceContainer space;
    TayStateStatus running;
} TayState;

void space_container_init(TaySpaceContainer *space,
                          void *storage,
                          int dims,
                          TAY_SPACE_DESTROY_FUNC destroy,
                          TAY_SPACE_ADD_FUNC add,
                          TAY_SPACE_SEE_FUNC see,
                          TAY_SPACE_ACT_FUNC act,
                          TAY_SPACE_UPDATE_FUNC update,
                          TAY_SPACE_SIM_START_FUNC on_simulation_start,
                          TAY_SPACE_SIM_END_FUNC on_simulation_end);
void space_simple_init(TaySpaceContainer *space, int dims);
void space_tree_init(TaySpaceContainer *space, int dims, float *radii, int max_depth_correction);
void space_gpu_simple_init(TaySpaceContainer *space, int dims);

/* Shared CPU version of see between linked lists of agents. */
void tay_see(struct TayAgentTag *seer_agents, struct TayAgentTag *seen_agents, TAY_SEE_FUNC func, float *radii, int dims, struct TayThreadContext *thread_context);

#endif
