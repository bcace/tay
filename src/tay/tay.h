#ifndef tay_h
#define tay_h


typedef struct TayAgent {
    struct TayAgent *prev, *next;
} TayAgent;

typedef enum TaySpaceType {
    TAY_SPACE_PLAIN,
} TaySpaceType;

struct TayState *tay_create_state(TaySpaceType space_type);
void tay_destroy_state(struct TayState *state);
int tay_add_group(struct TayState *state, int agent_size, int agent_capacity);
void tay_add_perception(struct TayState *state, int group1, int group2, void (*func)(void *, void *, void *));
void tay_add_action(struct TayState *state, int group, void (*func)(void *, void *));
void *tay_alloc_agent(struct TayState *state, int group_index);
void tay_run(struct TayState *state, int steps, void *context);

#endif
