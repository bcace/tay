#ifndef tay_space_impl_h
#define tay_space_impl_h

#include "state.h"

#define TAY_MAX_BUCKETS     32
#define TAY_GPU_NULL_INDEX  -1
#define TAY_GPU_DEAD_INDEX  -2
#define TAY_GPU_DEAD_ADDR   0xffffffffffffffff


void space_return_agents(Space *space, int group_i, TayAgentTag *tag, int is_point);

void space_see_point_point(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_nonpoint_point(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_point_nonpoint(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_nonpoint_nonpoint(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

void space_see_one_to_many_point_to_point(TayAgentTag *seer_agent, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_one_to_many_nonpoint_to_point(TayAgentTag *seer_agent, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_one_to_many_point_to_nonpoint(TayAgentTag *seer_agent, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_one_to_many_nonpoint_to_nonpoint(TayAgentTag *seer_agent, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

void cpu_simple_sort(Space *space, TayGroup *groups);
void cpu_simple_unsort(Space *space, TayGroup *groups);
void cpu_simple_see(TayPass *pass);
void cpu_simple_act(Space *space, TayPass *pass);

void cpu_tree_on_simulation_start(Space *space);
void cpu_tree_sort(Space *space, TayGroup *groups);
void cpu_tree_unsort(Space *space, TayGroup *groups);
void cpu_tree_single_space_see(Space *space, TayPass *pass);
void cpu_tree_see(TayPass *pass);
void cpu_tree_act(Space *space, TayPass *pass);

void cpu_grid_on_simulation_start(Space *space);
void cpu_grid_sort(Space *space, TayGroup *groups, TayPass *passes, int passes_count);
void cpu_grid_unsort(Space *space, TayGroup *groups);
void cpu_grid_see(TayPass *pass);
void cpu_grid_act(Space *space, TayPass *pass);

int space_agent_count_to_bucket_index(int count);

void box_reset(Box *box, int dims);
void box_update_from_agent(Box *box, TayAgentTag *agent, int dims, int is_point);

int group_tag_to_index(TayGroup *group, TayAgentTag *tag);

#endif
