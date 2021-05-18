#include "state.h"
#include "CL/cl.h"
#include <stdio.h>


unsigned ocl_simple_add_see_kernel_text(TayPass *pass, char *text, unsigned remaining_space, int dims) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
kernel void %s(global char *a_agents, global char *b_agents, constant void *c, float4 radii) {\n\
    unsigned a_i = get_global_id(0);\n\
    unsigned a_size = %d;\n\
    unsigned b_size = %d;\n\
    unsigned b_beg = 0;\n\
    unsigned b_end = %d;\n\
    %s\
    %s\
    %s\
\n\
    %s(a, b, c);\n\
    %s\
}\n\
\n",
    ocl_get_kernel_name(pass),
    pass->seer_group->agent_size,
    pass->seen_group->agent_size,
    pass->seen_group->space.count,
    ocl_pairing_prologue(pass->seer_group->is_point, pass->seen_group->is_point),
    ocl_self_see_text(pass->seer_group == pass->seen_group, pass->self_see),
    ocl_pairing_text(pass->seer_group->is_point, pass->seen_group->is_point, dims),
    pass->func_name,
    ocl_pairing_epilogue());

    return length;

    #else
    return 0u;
    #endif
}

unsigned ocl_simple_add_act_kernel_text(TayPass *pass, char *text, unsigned remaining_space) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
kernel void %s(global char *a, constant void *c) {\n\
    global void *agent = a + %d * get_global_id(0);\n\
    %s(agent, c);\n\
}\n\
\n",
    ocl_get_kernel_name(pass),
    pass->act_group->agent_size,
    pass->func_name);

    return length;

    #else
    return 0u;
    #endif
}

void ocl_simple_run_act_kernel(TayOcl *ocl, TayPass *pass) {
    #ifdef TAY_OCL

    cl_int err;

    err = clSetKernelArg(pass->pass_kernel, 0, sizeof(void *), &pass->act_group->space.ocl_simple.bridge.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 1, sizeof(void *), &pass->context_buffer);
    if (err)
        printf("clSetKernelArg error (context buffer)\n");

    unsigned long long global_work_size = pass->act_group->space.count;

    err = clEnqueueNDRangeKernel(ocl->queue,
                           pass->pass_kernel,
                           1,
                           0,
                           &global_work_size,
                           0,
                           0,
                           0,
                           0);
    if (err)
        printf("clEnqueueNDRangeKernel error\n");

    err = clFinish(ocl->queue);
    if (err)
        printf("clFinish error\n");

    #endif
}

void ocl_simple_run_see_kernel(TayOcl *ocl, TayPass *pass) {
    #ifdef TAY_OCL

    cl_int err;

    err = clSetKernelArg(pass->pass_kernel, 0, sizeof(void *), &pass->seer_group->space.ocl_simple.bridge.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 1, sizeof(void *), &pass->seen_group->space.ocl_simple.bridge.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 2, sizeof(void *), &pass->context_buffer);
    if (err)
        printf("clSetKernelArg error (context buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 3, sizeof(pass->radii), &pass->radii);
    if (err)
        printf("clSetKernelArg error (radii)\n");

    unsigned long long global_work_size = pass->seer_group->space.count;

    err = clEnqueueNDRangeKernel(ocl->queue,
                           pass->pass_kernel,
                           1,
                           0,
                           &global_work_size,
                           0,
                           0,
                           0,
                           0);
    if (err)
        printf("clEnqueueNDRangeKernel error\n");

    err = clFinish(ocl->queue);
    if (err)
        printf("clFinish error\n");

    #endif
}

void ocl_simple_get_kernel(TayOcl *ocl, TayPass *pass) {
    #ifdef TAY_OCL

    cl_int err;
    pass->pass_kernel = clCreateKernel(ocl->program, ocl_get_kernel_name(pass), &err);
    if (err)
        printf("clCreateKernel error\n");

    #endif
}
