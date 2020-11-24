#ifndef tay_space_impl_h
#define tay_space_impl_h

#include "state.h"

#define TAY_GPU_NULL_INDEX  -1
#define TAY_GPU_DEAD_INDEX  -2
#define TAY_GPU_DEAD_ADDR   0xffffffffffffffff


typedef struct TreeCell {
    struct TreeCell *lo, *hi;           /* child partitions */
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents contained in this cell (fork or leaf) */
    int dim;                            /* dimension along which the cell's partition is divided into child partitions */
    Box box;
    float mid;
} TreeCell;

void space_return_agents(Space *space, int group_i, TayAgentTag *tag);
void space_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

void cpu_simple_step(TayState *state);

void cpu_tree_step(TayState *state);

void space_gpu_shared_init(GpuShared *shared);
void space_gpu_shared_release(GpuShared *shared);
void space_gpu_on_simulation_start(TayState *state);
void space_gpu_on_simulation_end(TayState *state);
void space_gpu_push_agents(TayState *state);
void space_gpu_fetch_agents(TayState *state);
void space_gpu_shared_fix_gpu_pointers(TayState *state);
void space_gpu_fetch_agent_positions(TayState *state);

void gpu_simple_add_source(TayState *state);
void gpu_simple_on_simulation_start(TayState *state);
void gpu_simple_on_simulation_end(TayState *state);
void gpu_simple_step(TayState *state);

void gpu_tree_add_source(TayState *state);
void gpu_tree_on_simulation_start(TayState *state);
void gpu_tree_on_simulation_end(TayState *state);
void gpu_tree_step(TayState *state);

void tree_update(Space *space, Tree *tree);
void tree_return_agents(Space *space, Tree *tree);

void box_reset(Box *box, int dims);
void box_update(Box *box, float4 pos, int dims);

int group_tag_to_index(TayGroup *group, TayAgentTag *tag);

#endif
