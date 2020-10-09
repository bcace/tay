#ifndef tay_state_h
#define tay_state_h

#define TAY_MAX_GROUPS      8
#define TAY_MAX_PASSES      32
#define TAY_MAX_DIMENSIONS  8
#define TAY_INSTRUMENT      0

#define TAY_AGENT_POSITION(__agent__) ((float *)(__agent__ + 1))


typedef void (*TAY_SEE_FUNC)(void *, void *, void *);
typedef void (*TAY_ACT_FUNC)(void *, void *);

typedef void (*TAY_SPACE_DESTROY_FUNC)(struct TaySpace *space);
typedef void (*TAY_SPACE_ADD_FUNC)(struct TaySpace *space, struct TayAgent *agent, int group);
typedef void (*TAY_SPACE_SEE_FUNC)(struct TaySpace *space, struct TayPass *pass);
typedef void (*TAY_SPACE_ACT_FUNC)(struct TaySpace *space, struct TayPass *pass);
typedef void (*TAY_SPACE_UPDATE_FUNC)(struct TaySpace *space);

typedef struct TayGroup {
    void *storage;          /* agents storage */
    struct TayAgent *first; /* single linked list of available agents from storage */
    int size;               /* agent size in bytes */
    int capacity;           /* max. number of agents */
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
} TayPass;

typedef struct TaySpace {
    void *storage;
    int dims;
    TAY_SPACE_DESTROY_FUNC destroy;
    TAY_SPACE_ADD_FUNC add;
    TAY_SPACE_SEE_FUNC see;
    TAY_SPACE_ACT_FUNC act;
    TAY_SPACE_UPDATE_FUNC update;
} TaySpace;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    int passes_count;
    TaySpace space;
} TayState;

void space_init(TaySpace *space,
                void *storage,
                int dims,
                TAY_SPACE_DESTROY_FUNC destroy,
                TAY_SPACE_ADD_FUNC add,
                TAY_SPACE_SEE_FUNC see,
                TAY_SPACE_ACT_FUNC act,
                TAY_SPACE_UPDATE_FUNC update);
void space_simple_init(TaySpace *space, int dims);
void tree_init(TaySpace *space, int dims, float *radii, int max_depth_correction);
void grid_init(TaySpace *space, int dims, float *radii);
void tay_see(struct TayAgent *seer_agents, struct TayAgent *seen_agents, TAY_SEE_FUNC func, float *radii, int dims, struct TayThreadContext *thread_context);

#endif
