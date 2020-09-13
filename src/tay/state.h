#ifndef tay_state_h
#define tay_state_h

#define TAY_MAX_GROUPS      8
#define TAY_MAX_PASSES      32
#define TAY_MAX_DIMENSIONS  8

#define TAY_AGENT_POSITION(__agent__) ((float *)(__agent__ + 1))


typedef void (*TAY_SEE_FUNC)(void *, void *, void *);
typedef void (*TAY_ACT_FUNC)(void *, void *);

typedef struct TayGroup {
    void *storage;          /* agents storage */
    struct TayAgent *first; /* single linked list of available agents from storage */
    int size;               /* agent size in bytes */
    int capacity;           /* max. number of agents */
} TayGroup;

typedef enum TayPassType {
    TAY_PASS_SEE,
    TAY_PASS_ACT,
    TAY_PASS_POST,
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
} TayPass;

typedef struct TaySpace {
    void *storage;
    int dims;
    void (*destroy)(struct TaySpace *space);
    void (*add)(struct TaySpace *space, struct TayAgent *agent, int group);
    void (*see)(struct TaySpace *space, TayPass *pass, void *context);
    void (*act)(struct TaySpace *space, TayPass *pass, void *context);
    void (*post)(struct TaySpace *space, void (*func)(void *), void *context);
    void (*iter)(struct TaySpace *space, int group, void (*func)(void *, void *), void *context);
    void (*update)(struct TaySpace *space);
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
                void (*destroy)(TaySpace *space),
                void (*add)(TaySpace *space, struct TayAgent *agent, int group),
                void (*see)(TaySpace *space, TayPass *pass, void *context),
                void (*act)(TaySpace *space, TayPass *pass, void *context),
                void (*post)(TaySpace *space, void (*func)(void *), void *context),
                void (*iter)(TaySpace *space, int group, void (*func)(void *, void *), void *context),
                void (*update)(TaySpace *space));
void space_simple_init(TaySpace *space, int dims);
void tree_init(TaySpace *space, int dims, float *radii, int max_depth_correction);
void tay_see(struct TayAgent *seer_agents, struct TayAgent *seen_agents, TAY_SEE_FUNC func, float *radii, int dims, struct TayThreadContext *thread_context);

#endif
