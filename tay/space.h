#ifndef tay_space_impl_h
#define tay_space_impl_h

#include "state.h"

#define TAY_MAX_BUCKETS     32


void space_return_agents(Space *space, TayAgentTag *tag, int is_point);

void space_see_point_point(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_point_point_self_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_nonpoint_point(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_point_nonpoint(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_nonpoint_nonpoint(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_nonpoint_nonpoint_self_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

void cpu_simple_sort(TayGroup *group);
void cpu_simple_unsort(TayGroup *group);
void cpu_simple_see(TayPass *pass);
void cpu_simple_act(TayPass *pass);
void cpu_simple_see_seen(TayPass *pass, TayAgentTag *seer_agents, Box seer_box, int dims, struct TayThreadContext *thread_context);

void cpu_tree_on_simulation_start(Space *space);
void cpu_tree_sort(TayGroup *group);
void cpu_tree_unsort(TayGroup *group);
void cpu_tree_see(TayPass *pass);
void cpu_tree_act(TayPass *pass);
void cpu_kd_tree_see_seen(TayPass *pass, TayAgentTag *seer_agents, Box seer_box, int dims, struct TayThreadContext *thread_context);

void cpu_aabb_tree_sort(TayGroup *group);
void cpu_aabb_tree_unsort(TayGroup *group);
void cpu_aabb_tree_see(TayPass *pass);
void cpu_aabb_tree_act(TayPass *pass);
void cpu_aabb_tree_see_seen(TayPass *pass, TayAgentTag *seer_agents, Box seer_box, int dims, struct TayThreadContext *thread_context);

void cpu_grid_on_simulation_start(Space *space);
void cpu_grid_sort(TayGroup *group);
void cpu_grid_unsort(TayGroup *group);
void cpu_grid_see(TayPass *pass);
void cpu_grid_act(TayPass *pass);
void cpu_grid_see_seen(TayPass *pass, TayAgentTag *seer_agents, Box seer_box, int dims, struct TayThreadContext *thread_context);

void cpu_hash_grid_on_simulation_start(Space *space);
void cpu_hash_grid_sort(TayGroup *group, TayPass *passes, int passes_count);
void cpu_hash_grid_unsort(TayGroup *group);
void cpu_hash_grid_see(TayPass *pass);
void cpu_hash_grid_act(TayPass *pass);

int space_agent_count_to_bucket_index(int count);

void box_reset(Box *box, int dims);
void box_update_from_agent(Box *box, TayAgentTag *agent, int dims, int is_point);

#endif
