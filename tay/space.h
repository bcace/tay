#ifndef tay_space_impl_h
#define tay_space_impl_h

#include "state.h"

#define TAY_MAX_BUCKETS     32
#define TAY_GPU_NULL_INDEX  -1
#define TAY_GPU_DEAD_INDEX  -2
#define TAY_GPU_DEAD_ADDR   0xffffffffffffffff


void space_return_agents(Space *space, int group_i, TayAgentTag *tag);
void space_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_single_seer_see(TayAgentTag *seer_agent, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

void cpu_simple_sort(Space *space, TayGroup *groups);
void cpu_simple_unsort(Space *space, TayGroup *groups);
void cpu_simple_single_space_see(Space *space, TayPass *pass);
void cpu_simple_act(Space *space, TayPass *pass);

void cpu_tree_on_type_switch(Space *space);
void cpu_tree_sort(Space *space, TayGroup *groups);
void cpu_tree_unsort(Space *space, TayGroup *groups);
void cpu_tree_single_space_see(Space *space, TayPass *pass);
void cpu_tree_act(Space *space, TayPass *pass);

void cpu_grid_on_type_switch(Space *space);
void cpu_grid_sort(Space *space, TayGroup *groups, TayPass *passes, int passes_count);
void cpu_grid_unsort(Space *space, TayGroup *groups);
void cpu_grid_single_space_see(Space *space, TayPass *pass);
void cpu_grid_act(Space *space, TayPass *pass);

#if TAY_GPU
// void space_gpu_on_simulation_start(TayState *state);
// void space_gpu_on_simulation_end(TayState *state);
void gpu_shared_push_agents_and_pass_contexts(TayState *state);
void space_gpu_fetch_agents(TayState *state);
// void space_gpu_finish_fixing_group_gpu_pointers(GpuShared *shared, TayGroup *group, int group_i, int *next_indices);
// void space_gpu_fetch_agent_positions(TayState *state);

void gpu_simple_add_source(TayState *state);
void gpu_simple_create_kernels(TayState *state);
void gpu_simple_destroy_kernels(TayState *state);
void gpu_simple_create_io_buffer(Space *space, GpuShared *shared);
void gpu_simple_destroy_io_buffer(Space *space);
// void gpu_simple_fix_gpu_pointers(TayState *state);
// void gpu_simple_step(TayState *state, int direct);
#endif

int space_agent_count_to_bucket_index(int count);

void box_reset(Box *box, int dims);
void box_update(Box *box, float4 pos, int dims);

int group_tag_to_index(TayGroup *group, TayAgentTag *tag);

#endif
