#include "state.h"
#include "CL/cl.h"
#include <stdio.h>


unsigned ocl_simple_add_see_kernel_text(TayPass *pass, char *text, unsigned remaining_space) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
kernel void %s_kernel(global char *agents_a, global char *agents_b, constant void *c) {\n\
    global void *a = agents_a + %d * get_global_id(0);\n\
    for (unsigned b_i = 0; b_i < %d; ++b_i) {\n\
        global void *b = agents_b + %d * b_i;\n\
        %s(a, b, c);\n\
    }\n\
}\n\
\n",
    pass->func_name, pass->seer_group->agent_size, pass->seen_group->space.count, pass->seen_group->agent_size, pass->func_name);

    return length;

    #else
    return 0u;
    #endif
}

unsigned ocl_simple_add_act_kernel_text(TayPass *pass, char *text, unsigned remaining_space) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
kernel void %s_kernel(global char *a, constant void *c) {\n\
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

void ocl_simple_run_act_kernel(TayOcl *ocl, TayPass *pass) {
    #ifdef TAY_OCL

    cl_int err;

    err = clSetKernelArg(pass->pass_kernel, 0, sizeof(void *), &pass->act_group->space.ocl_simple.agent_buffer);
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

    err = clSetKernelArg(pass->pass_kernel, 0, sizeof(void *), &pass->seer_group->space.ocl_simple.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 1, sizeof(void *), &pass->seen_group->space.ocl_simple.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 2, sizeof(void *), &pass->context_buffer);
    if (err)
        printf("clSetKernelArg error (context buffer)\n");

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

    char kernel_name[TAY_MAX_FUNC_NAME + 64];
    sprintf_s(kernel_name, TAY_MAX_FUNC_NAME + 64, "%s_kernel", pass->func_name);

    cl_int err;
    pass->pass_kernel = clCreateKernel(ocl->program, kernel_name, &err);
    if (err)
        printf("clCreateKernel error\n");

    #endif
}
