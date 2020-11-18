#ifndef tay_space_impl_h
#define tay_space_impl_h

#include "state.h"


typedef struct TreeCell {
    struct TreeCell *lo, *hi;           /* child partitions */
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents contained in this cell (fork or leaf) */
    int dim;                            /* dimension along which the cell's partition is divided into child partitions */
    Box box;
    float mid;
} TreeCell;

void space_return_agents(struct Space *space, int group_i, TayAgentTag *tag);

void cpu_simple_step(struct TayState *state);
void cpu_tree_step(struct TayState *state);

void space_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

void tree_update(Space *space);
void tree_return_agents(Space *space);

void box_reset(Box *box, int dims);
void box_update(Box *box, float4 pos, int dims);

#endif
