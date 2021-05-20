#include "state.h"
#include "CL/cl.h"
#include <stdio.h>


unsigned ocl_grid_add_sort_kernel_text(char *text, unsigned remaining_space) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
typedef struct {\n\
    float4 min;\n\
    float4 max;\n\
} Box;\n\
\n\
kernel void grid_sort_kernel(global char *agents, unsigned agent_size, unsigned agents_count, global char *space_buffer, local char *local_buffer) {\n\
    unsigned thread_i = get_global_id(0);\n\
    unsigned threads_count = get_global_size(0);\n\
    unsigned loc_i = get_local_id(0);\n\
    unsigned loc_count = get_local_size(0);\n\
\n\
    global Box *boxes = (global Box *)space_buffer;\n\
    local Box *lboxes = (local Box *)local_buffer;\n\
\n\
    float4 min = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};\n\
    float4 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};\n\
    for (unsigned a_i = thread_i; a_i < agents_count; a_i += threads_count) {\n\
        float4 a_p = float4_agent_position(agents + agent_size * a_i);\n\
        if (a_p.x < min.x)\n\
            min.x = a_p.x;\n\
        if (a_p.x > max.x)\n\
            max.x = a_p.x;\n\
        if (a_p.y < min.y)\n\
            min.y = a_p.y;\n\
        if (a_p.y > max.y)\n\
            max.y = a_p.y;\n\
        if (a_p.z < min.z)\n\
            min.z = a_p.z;\n\
        if (a_p.z > max.z)\n\
            max.z = a_p.z;\n\
    }\n\
    lboxes[loc_i].min = min;\n\
    lboxes[loc_i].max = max;\n\
\n\
    barrier(CLK_LOCAL_MEM_FENCE);\n\
    if (loc_i == 0) {\n\
        Box box = lboxes[loc_i];\n\
        for (unsigned loc_j = 1; loc_j < loc_count; ++loc_j) {\n\
            Box other_box = lboxes[loc_j];\n\
            if (other_box.min.x < box.min.x)\n\
                box.min.x = other_box.min.x;\n\
            if (other_box.max.x > box.max.x)\n\
                box.max.x = other_box.max.x;\n\
            if (other_box.min.y < box.min.y)\n\
                box.min.y = other_box.min.y;\n\
            if (other_box.max.y > box.max.y)\n\
                box.max.y = other_box.max.y;\n\
            if (other_box.min.z < box.min.z)\n\
                box.min.z = other_box.min.z;\n\
            if (other_box.max.z > box.max.z)\n\
                box.max.z = other_box.max.z;\n\
        }\n\
        boxes[get_group_id(0)] = box;\n\
    }\n\
}\n\
\n\
kernel void grid_sort_kernel_2(global char *space_buffer, unsigned boxes_count) {\n\
    global Box *boxes = (global Box *)space_buffer;\n\
    Box box = boxes[0];\n\
    for (unsigned i = 1; i < boxes_count; ++i) {\n\
        Box other_box = boxes[i];\n\
        if (other_box.min.x < box.min.x)\n\
            box.min.x = other_box.min.x;\n\
        if (other_box.max.x > box.max.x)\n\
            box.max.x = other_box.max.x;\n\
        if (other_box.min.y < box.min.y)\n\
            box.min.y = other_box.min.y;\n\
        if (other_box.max.y > box.max.y)\n\
            box.max.y = other_box.max.y;\n\
        if (other_box.min.z < box.min.z)\n\
            box.min.z = other_box.min.z;\n\
        if (other_box.max.z > box.max.z)\n\
            box.max.z = other_box.max.z;\n\
    }\n\
    boxes[0] = box;\n\
}\n\
\n");

    return length;

    #else
    return 0u;
    #endif
}

void ocl_grid_run_sort_kernel(TayState *state, TayGroup *group) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    err = clSetKernelArg(ocl->grid_sort_kernel, 0, sizeof(void *), &group->space.ocl_common.agent_buffer);
    if (err)
        printf("clSetKernelArg error (grid_sort_kernel agent buffer)\n");

    err = clSetKernelArg(ocl->grid_sort_kernel, 1, sizeof(group->agent_size), &group->agent_size);
    if (err)
        printf("clSetKernelArg error (grid_sort_kernel agent size)\n");

    err = clSetKernelArg(ocl->grid_sort_kernel, 2, sizeof(group->space.count), &group->space.count);
    if (err)
        printf("clSetKernelArg error (grid_sort_kernel agents count)\n");

    err = clSetKernelArg(ocl->grid_sort_kernel, 3, sizeof(void *), &group->space.ocl_common.space_buffer);
    if (err)
        printf("clSetKernelArg error (grid_sort_kernel space buffer)\n");

    err = clSetKernelArg(ocl->grid_sort_kernel, 4, 64 * sizeof(Box), (void *)NULL);
    if (err)
        printf("clSetKernelArg error (local buffer)\n");

    unsigned long long global_work_size = 1024 * 1;
    unsigned long long local_work_size = 64;
    unsigned work_groups_count = (unsigned)(global_work_size / local_work_size);

    err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernel, 1, 0, &global_work_size, &local_work_size, 0, 0, 0);
    if (err)
        printf("clEnqueueNDRangeKernel error (grid_sort_kernel)\n");

    err = clSetKernelArg(ocl->grid_sort_kernel_2, 0, sizeof(void *), &group->space.ocl_common.space_buffer);
    if (err)
        printf("clSetKernelArg error (grid_sort_kernel_2 space buffer)\n");

    err = clSetKernelArg(ocl->grid_sort_kernel_2, 1, sizeof(work_groups_count), &work_groups_count);
    if (err)
        printf("clSetKernelArg error (grid_sort_kernel_2 work_groups_count)\n");

    unsigned long long one = 1;

    err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernel_2, 1, 0, &one, 0, 0, 0, 0);
    if (err)
        printf("clEnqueueNDRangeKernel error (grid_sort_kernel_2)\n");

    clFinish(ocl->queue);

    #endif
}

unsigned ocl_grid_add_see_kernel_text(TayPass *pass, char *text, unsigned remaining_space, int dims) {
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
