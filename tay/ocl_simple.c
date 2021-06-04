#include "state.h"
#include "CL/cl.h"
#include <stdio.h>


void ocl_simple_add_seen_text(OclText *text, TayPass *pass, int dims) {
    ocl_text_append(text, "{ /* simple seen loop */\n\
    const unsigned b_beg = 0;\n\
    const unsigned b_end = %d;\n\
\n\
%s\
} /* simple seen loop */\n", pass->seen_group->space.count, ocl_get_coupling_text(pass, dims));
}

void ocl_simple_add_see_kernel_text(OclText *text, TayPass *pass, int dims) {
    ocl_text_append(text, "\n\
kernel void %s(global char *a_agents, global char *b_agents, constant void *c, float4 radii, global void *space_buffer) {\n\
    const unsigned a_size = %d;\n\
    const unsigned b_size = %d;\n\
    const int dims = %d;\n\
\n\
    unsigned a_i = get_global_id(0);\n\
%s\
\n",
    ocl_get_kernel_name(pass),
    pass->seer_group->agent_size,
    pass->seen_group->agent_size,
    dims,
    ocl_get_seer_agent_text(pass));

    ocl_add_seen_text(text, pass, dims);
    ocl_text_append(text, "}\n");
}
