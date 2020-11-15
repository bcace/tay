#ifndef tay_space_impl_h
#define tay_space_impl_h


void space_return_agents(struct Space *space, int group_i, struct TayAgentTag *tag);

void cpu_simple_step(struct TayState *state);
void cpu_tree_step(struct TayState *state);

#endif
