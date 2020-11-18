#ifndef tay_space_impl_h
#define tay_space_impl_h

#include "state.h"


void space_return_agents(struct Space *space, int group_i, TayAgentTag *tag);

void cpu_simple_step(struct TayState *state);
void cpu_tree_step(struct TayState *state);

void space_see(TayAgentTag *seer_agents, TayAgentTag *seen_agents, TAY_SEE_FUNC func, float4 radii, int dims, struct TayThreadContext *thread_context);

void tree_update(Space *space);
void tree_return_agents(Space *space);

void box_reset(Box *box, int dims);
void box_update(Box *box, float4 pos, int dims);

#endif
