#ifndef tay_state_h
#define tay_state_h

#define TAY_MAX_GROUPS 32
#define TAY_MAX_PASSES 32


typedef void (*TAY_ACTION_FUNC)(void *, void *);
typedef void (*TAY_PERCEPTION_FUNC)(void *, void *, void *);

typedef struct TayAgent {
    struct TayAgent *prev, *next;
} TayAgent;

typedef struct TayGroup {
    void *storage;      /* agents storage */
    TayAgent *first;    /* single linked list of available agents from storage */
    int size;           /* agent size in bytes */
    int capacity;       /* max. number of agents */
} TayGroup;

typedef struct TayPass {
    int group1, group2;
    TAY_ACTION_FUNC action;
    TAY_PERCEPTION_FUNC perception;
} TayPass;

typedef struct TaySpace {
    void *storage;
    void (*destroy)(struct TaySpace *space);
    void (*add)(struct TaySpace *space, TayAgent *agent, int group);
    void (*perception)(struct TaySpace *space, int group1, int group2, TAY_PERCEPTION_FUNC func, void *context);
    void (*action)(struct TaySpace *space, int group, TAY_ACTION_FUNC func, void *context);
} TaySpace;

typedef enum TaySpaceType {
    TAY_SPACE_PLAIN,
} TaySpaceType;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    int passes_count;
    TaySpace space;
} TayState;

TayState *tay_create_state(TaySpaceType space_type);
void tay_destroy_state(TayState *state);

int tay_add_group(TayState *state, int agent_size, int agent_capacity);
void tay_add_perception(TayState *state, int group1, int group2, TAY_PERCEPTION_FUNC func);
void tay_add_action(TayState *state, int group, TAY_ACTION_FUNC func);
void *tay_alloc_agent(TayState *state, int group_index);

void tay_run(TayState *state, int steps, void *context);

void space_plain_init(TaySpace *space);

#endif
