#ifndef tay_space_impl_h
#define tay_space_impl_h

#include "state.h"

#define TAY_MAX_BUCKETS     32


void space_update_box(TayGroup *group);

void space_see_point_point(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_point_point_self_see(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_nonpoint_point(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_point_nonpoint(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_nonpoint_nonpoint(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);
void space_see_nonpoint_nonpoint_self_see(AgentsSlice seer_slice, AgentsSlice seen_slice, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

void cpu_simple_sort(TayGroup *group);
void cpu_simple_unsort(TayGroup *group);
void cpu_simple_see(TayPass *pass);
void cpu_simple_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, struct TayThreadContext *thread_context);

void cpu_tree_on_simulation_start(Space *space);
void cpu_tree_sort(TayGroup *group);
void cpu_tree_unsort(TayGroup *group);
void cpu_tree_see(TayPass *pass);
void cpu_kd_tree_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, struct TayThreadContext *thread_context);

void cpu_aabb_tree_sort(TayGroup *group);
void cpu_aabb_tree_unsort(TayGroup *group);
void cpu_aabb_tree_see(TayPass *pass);
void cpu_aabb_tree_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, struct TayThreadContext *thread_context);

void cpu_grid_on_simulation_start(Space *space);
void cpu_grid_sort(TayGroup *group);
void cpu_grid_unsort(TayGroup *group);
void cpu_grid_see(TayPass *pass);
void cpu_grid_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, struct TayThreadContext *thread_context);

void cpu_z_grid_on_simulation_start(Space *space);
void cpu_z_grid_sort(TayGroup *group);
void cpu_z_grid_unsort(TayGroup *group);
void cpu_z_grid_see(TayPass *pass);
void cpu_z_grid_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, struct TayThreadContext *thread_context);

unsigned ocl_simple_add_see_kernel_text(TayPass *pass, char *text, unsigned remaining_space);
unsigned ocl_simple_add_act_kernel_text(TayPass *pass, char *text, unsigned remaining_space);
void ocl_simple_run_act_kernel(TayOcl *ocl, TayPass *pass);

void ocl_simple_get_kernel(TayOcl *ocl, TayPass *pass);

int space_agent_count_to_bucket_index(int count);

void box_reset(Box *box, int dims);
void box_update_from_agent(Box *box, char *agent, int dims, int is_point);

void space_act(TayPass *pass);
void space_run_thread_tasks(TayPass *pass, void (*task_func)(struct TayThreadTask *, struct TayThreadContext *));

#endif
