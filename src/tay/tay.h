#ifndef tay_h
#define tay_h


typedef enum TaySpaceType {
    TAY_SPACE_CPU_SIMPLE,
    TAY_SPACE_CPU_TREE,
    TAY_SPACE_GPU_SIMPLE,
    TAY_SPACE_GPU_TREE,
} TaySpaceType;

typedef struct TayState TayState;

TayState *tay_create_state(int space_dims, struct float4 see_radii);
TayState *tay_create_state_specific(int space_dims, struct float4 see_radii, int initial_space_type, int max_depth_correction);
void tay_destroy_state(TayState *state);

void tay_set_source(TayState *state, const char *source);

int tay_add_group(TayState *state, int agent_size, int agent_capacity);
void tay_add_see(TayState *state, int seer_group, int seen_group, void (*func)(void *, void *, void *), const char *func_name, struct float4 radii, void *context, int context_size);
void tay_add_act(TayState *state, int act_group, void (*func)(void *, void *), const char *func_name, void *context, int context_size);

void *tay_get_available_agent(TayState *state, int group);
void tay_commit_available_agent(TayState *state, int group);
void *tay_get_agent(TayState *state, int group, int index);

void tay_simulation_start(TayState *state);
void tay_run(TayState *state, int steps);
void tay_simulation_end(TayState *state);

#endif
