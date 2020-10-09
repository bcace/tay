#ifndef tay_h
#define tay_h


typedef struct TayAgent {
    struct TayAgent *next;
} TayAgent;

typedef enum TaySpaceType {
    TAY_SPACE_SIMPLE,
    TAY_SPACE_TREE,
    TAY_SPACE_GRID,
} TaySpaceType;

typedef struct TayState TayState;

TayState *tay_create_state(TaySpaceType space_type, int space_dimensions, float *space_radii, int max_depth_correction);
void tay_destroy_state(TayState *state);
int tay_add_group(TayState *state, int agent_size, int agent_capacity);
void tay_add_see(TayState *state, int seer_group, int seen_group, void (*func)(void *, void *, void *), float *radii, void *context);
void tay_add_act(TayState *state, int act_group, void (*func)(void *, void *), void *context);
void *tay_get_new_agent(TayState *state, int group);
void tay_add_new_agent(TayState *state, int group);
void *tay_get_storage(TayState *state, int group);
void tay_run(TayState *state, int steps);

#endif
