#include "state.h"
#include <stdio.h>


unsigned ocl_add_see_kernel_text(TayPass *pass, char *text, unsigned remaining_space) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
void %s_kernel(global char *a, global char *b, global void *c) {\n\
    %s((global void *)a, (global void *)b, c);\n\
}\n\
\n",
    pass->func_name, pass->func_name);

    return length;

    #else
    return 0u;
    #endif
}

unsigned ocl_add_act_kernel_text(TayPass *pass, char *text, unsigned remaining_space) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
void %s_kernel(global char *a, global void *c) {\n\
    global void *agent = a + %d * get_global_id(0);\n\
    %s(agent, c);\n\
}\n\
\n",
    pass->func_name, pass->act_group->agent_size, pass->func_name);

    return length;

    #else
    return 0u;
    #endif
}
