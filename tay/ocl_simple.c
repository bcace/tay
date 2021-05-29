#include "state.h"
#include "CL/cl.h"
#include <stdio.h>


void ocl_simple_add_see_kernel_text(TayPass *pass, OclText *text, int dims) {
    #ifdef TAY_OCL

    ocl_text_append(text, "\n\
kernel void %s(global char *a_agents, global char *b_agents, constant void *c, float4 radii) {\n\
    unsigned a_i = get_global_id(0);\n\
    const unsigned a_size = %d;\n\
    const unsigned b_size = %d;\n\
    const unsigned b_beg = 0;\n\
    const unsigned b_end = %d;\n\
\n\
%s\
\n\
%s\
}\n\
\n",
    ocl_get_kernel_name(pass),
    pass->seer_group->agent_size,
    pass->seen_group->agent_size,
    pass->seen_group->space.count,
    ocl_get_seer_agent_text(pass),
    ocl_get_coupling_text(pass, dims));

    #endif
}

void ocl_simple_run_see_kernel(TayOcl *ocl, TayPass *pass) {
    #ifdef TAY_OCL

    cl_int err;

    err = clSetKernelArg(pass->pass_kernel, 0, sizeof(void *), &pass->seer_group->space.ocl_common.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 1, sizeof(void *), &pass->seen_group->space.ocl_common.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 2, sizeof(void *), &pass->context_buffer);
    if (err)
        printf("clSetKernelArg error (context buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 3, sizeof(pass->radii), &pass->radii);
    if (err)
        printf("clSetKernelArg error (radii)\n");

    unsigned long long global_work_size = pass->seer_group->space.count;

    err = clEnqueueNDRangeKernel(ocl->queue, pass->pass_kernel, 1, 0, &global_work_size, 0, 0, 0, 0);
    if (err)
        printf("clEnqueueNDRangeKernel error\n");

    err = clFinish(ocl->queue);
    if (err)
        printf("clFinish error\n");

    #endif
}
