#include "state.h"
#include "CL/cl.h"
#include <stdio.h>


#pragma pack(push, 1)

typedef struct {
    unsigned count;
    unsigned offset;
} OclGridCell;

typedef struct {
    float4 origin;
    float4 cell_sizes;
    int4 cell_counts;
    unsigned cells_count;
    OclGridCell cells[];
} OclGrid;

#pragma pack(pop)

unsigned ocl_grid_add_kernel_texts(char *text, unsigned remaining_space) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
typedef struct {\n\
    float4 min;\n\
    float4 max;\n\
} Box;\n\
\n\
typedef struct {\n\
    unsigned count;\n\
    unsigned offset;\n\
} OclGridCell;\n\
\n\
typedef struct {\n\
    float4 origin;\n\
    float4 cell_sizes;\n\
    int4 cell_counts;\n\
    unsigned cells_count;\n\
    OclGridCell cells[1];\n\
} OclGrid;\n\
\n\
kernel void grid_sort_kernel_0(global char *agents, unsigned agent_size, unsigned agents_count, global Box *boxes) {\n\
    unsigned thread_i = get_global_id(0);\n\
    unsigned threads_count = get_global_size(0);\n\
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
    boxes[thread_i].min = min;\n\
    boxes[thread_i].max = max;\n\
}\n\
unsigned grid_z_cell_indices_to_cell_index(int4 idx, int dims) {\n\
    if (dims == 1) {\n\
        return idx.x;\n\
    }\n\
    else if (dims ==  2) { /* 16 bits */\n\
        unsigned x = idx.x;\n\
        unsigned y = idx.y;\n\
\n\
        x = (x | (x << 8)) & 0x00FF00FF;\n\
        x = (x | (x << 4)) & 0x0F0F0F0F;\n\
        x = (x | (x << 2)) & 0x33333333;\n\
        x = (x | (x << 1)) & 0x55555555;\n\
\n\
        y = (y | (y << 8)) & 0x00FF00FF;\n\
        y = (y | (y << 4)) & 0x0F0F0F0F;\n\
        y = (y | (y << 2)) & 0x33333333;\n\
        y = (y | (y << 1)) & 0x55555555;\n\
\n\
        return x | (y << 1);\n\
    }\n\
    else if (dims == 3) { /* 10 bits */\n\
        unsigned x = idx.x;\n\
        unsigned y = idx.y;\n\
        unsigned z = idx.z;\n\
\n\
        x = (x | (x << 16)) & 0x030000FF;\n\
        x = (x | (x <<  8)) & 0x0300F00F;\n\
        x = (x | (x <<  4)) & 0x030C30C3;\n\
        x = (x | (x <<  2)) & 0x09249249;\n\
\n\
        y = (y | (y << 16)) & 0x030000FF;\n\
        y = (y | (y <<  8)) & 0x0300F00F;\n\
        y = (y | (y <<  4)) & 0x030C30C3;\n\
        y = (y | (y <<  2)) & 0x09249249;\n\
\n\
        z = (z | (z << 16)) & 0x030000FF;\n\
        z = (z | (z <<  8)) & 0x0300F00F;\n\
        z = (z | (z <<  4)) & 0x030C30C3;\n\
        z = (z | (z <<  2)) & 0x09249249;\n\
\n\
        return x | (y << 1) | (z << 2);\n\
    }\n\
    else {\n\
        /* not implemented */\n\
        return 0;\n\
    }\n\
}\n\
\n\
\n\
kernel void grid_sort_kernel_1(global void *space_buffer, unsigned boxes_count, int dims, float4 min_part_sizes) {\n\
    global Box *boxes = space_buffer;\n\
    float4 min = boxes[0].min;\n\
    float4 max = boxes[0].max;\n\
    boxes[0].min = (float4)(0.0f);\n\
    boxes[0].max = (float4)(0.0f);\n\
    for (unsigned i = 1; i < boxes_count; ++i) {\n\
        min = fmin(min, boxes[i].min);\n\
        max = fmax(max, boxes[i].max);\n\
        boxes[i].min = (float4)(0.0f);\n\
        boxes[i].max = (float4)(0.0f);\n\
    }\n\
\n\
    global OclGrid *grid = space_buffer;\n\
    grid->origin = min;\n\
\n\
    float4 cell_size = min_part_sizes;\n\
    float4 space_size = max - min + cell_size * 0.001f;\n\
    grid->cell_counts = convert_int4(floor(space_size / cell_size));\n\
    grid->cell_sizes = space_size / convert_float4(grid->cell_counts);\n\
\n\
    // grid->cells_count = grid_z_cell_indices_to_cell_index(grid->cell_counts, dims) + 1;\n\
    if (dims == 1)\n\
        grid->cells_count = grid->cell_counts.x;\n\
    else if (dims == 2)\n\
        grid->cells_count = grid->cell_counts.x * grid->cell_counts.y;\n\
    else if (dims == 3)\n\
        grid->cells_count = grid->cell_counts.x * grid->cell_counts.y * grid->cell_counts.z;\n\
    else\n\
        grid->cells_count = grid->cell_counts.x * grid->cell_counts.y * grid->cell_counts.z * grid->cell_counts.w;\n\
}\n\
\n\
unsigned grid_cell_indices_to_cell_index(int4 indices, int4 counts, int dims) {\n\
    switch (dims) {\n\
        case 1: {\n\
            return indices.x;\n\
        } break;\n\
        case 2: {\n\
            return indices.y * counts.x +\n\
                   indices.x;\n\
        } break;\n\
        case 3: {\n\
            return indices.z * counts.x * counts.y +\n\
                   indices.y * counts.x +\n\
                   indices.x;\n\
        } break;\n\
        default: {\n\
            return indices.w * counts.x * counts.y * counts.z +\n\
                   indices.z * counts.x * counts.y +\n\
                   indices.y * counts.x +\n\
                   indices.x;\n\
        };\n\
    }\n\
}\n\
\n\
kernel void grid_sort_kernel_2(global char *agents, unsigned agent_size, global OclGrid *grid, int dims) {\n\
    global TayAgentTag *a = (global TayAgentTag *)(agents + get_global_id(0) * agent_size);\n\
    float4 p = float4_agent_position(a);\n\
\n\
    int4 indices = convert_int4(floor((p - grid->origin) / grid->cell_sizes));\n\
    // unsigned cell_i = grid_z_cell_indices_to_cell_index(indices, dims);\n\
    unsigned cell_i = grid_cell_indices_to_cell_index(indices, grid->cell_counts, dims);\n\
\n\
    a->part_i = cell_i;\n\
    a->cell_agent_i = atomic_inc(&grid->cells[cell_i].count);\n\
}\n\
\n\
kernel void grid_sort_kernel_3(global OclGrid *grid) {\n\
    unsigned thread_i = get_global_id(0);\n\
    unsigned threads_count = get_global_size(0);\n\
\n\
    unsigned per_thread_count = grid->cells_count / threads_count;\n\
    unsigned beg = per_thread_count * thread_i;\n\
    unsigned end = (thread_i < threads_count - 1u) ? beg + per_thread_count : grid->cells_count;\n\
\n\
    unsigned count = 0;\n\
    for (unsigned cell_i = beg; cell_i < end; ++cell_i) {\n\
        grid->cells[cell_i].offset = count;\n\
        count += grid->cells[cell_i].count;\n\
    }\n\
    grid->cells[beg].offset = count;\n\
}\n\
\n\
kernel void grid_sort_kernel_4(global OclGrid *grid, unsigned threads_count) {\n\
    unsigned per_thread_count = grid->cells_count / threads_count;\n\
\n\
    unsigned prev_offset = grid->cells[0].offset;\n\
    grid->cells[0].offset = 0;\n\
    for (unsigned thread_i = 1; thread_i < threads_count; ++thread_i) {\n\
        unsigned beg = per_thread_count * thread_i;\n\
        unsigned offset = grid->cells[beg].offset;\n\
        grid->cells[beg].offset = prev_offset;\n\
        prev_offset += offset;\n\
    }\n\
}\n\
\n\
kernel void grid_sort_kernel_5(global OclGrid *grid) {\n\
    unsigned thread_i = get_global_id(0);\n\
    unsigned threads_count = get_global_size(0);\n\
\n\
    unsigned per_thread_count = grid->cells_count / threads_count;\n\
    unsigned beg = per_thread_count * thread_i;\n\
    unsigned end = (thread_i < threads_count - 1u) ? beg + per_thread_count : grid->cells_count;\n\
\n\
    unsigned beg_offset = grid->cells[beg].offset;\n\
    for (unsigned cell_i = beg + 1; cell_i < end; ++cell_i)\n\
        grid->cells[cell_i].offset += beg_offset;\n\
}\n\
\n\
kernel void grid_sort_kernel_6(global char *agent_buffer_a, global char *agent_buffer_b, unsigned agent_size, global OclGrid *grid) {\n\
    unsigned a_i = get_global_id(0);\n\
    global TayAgentTag *a = (global TayAgentTag *)(agent_buffer_a + a_i * agent_size);\n\
    global char *b = agent_buffer_b + (grid->cells[a->part_i].offset + a->cell_agent_i) * agent_size;\n\
    tay_memcpy(b, (global char *)a, agent_size);\n\
}\n\
\n\
kernel void grid_reset_cells(global OclGrid *grid) {\n\
    unsigned thread_i = get_global_id(0);\n\
    unsigned threads_count = get_global_size(0);\n\
\n\
    unsigned per_thread_count = grid->cells_count / threads_count;\n\
    unsigned beg = per_thread_count * thread_i;\n\
    unsigned end = (thread_i < threads_count - 1u) ? beg + per_thread_count : grid->cells_count;\n\
\n\
    for (unsigned cell_i = beg; cell_i < end; ++cell_i)\n\
        grid->cells[cell_i].count = 0;\n\
}\n\
\n");

    return length;

    #else
    return 0u;
    #endif
}

void ocl_grid_get_kernels(TayState *state) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    ocl->grid_sort_kernels[0] = clCreateKernel(ocl->program, "grid_sort_kernel_0", &err);
    if (err)
        printf("clCreateKernel error (grid_sort_kernel_0)\n");

    ocl->grid_sort_kernels[1] = clCreateKernel(ocl->program, "grid_sort_kernel_1", &err);
    if (err)
        printf("clCreateKernel error (grid_sort_kernel_1)\n");

    ocl->grid_sort_kernels[2] = clCreateKernel(ocl->program, "grid_sort_kernel_2", &err);
    if (err)
        printf("clCreateKernel error (grid_sort_kernel_2)\n");

    ocl->grid_sort_kernels[3] = clCreateKernel(ocl->program, "grid_sort_kernel_3", &err);
    if (err)
        printf("clCreateKernel error (grid_sort_kernel_3)\n");

    ocl->grid_sort_kernels[4] = clCreateKernel(ocl->program, "grid_sort_kernel_4", &err);
    if (err)
        printf("clCreateKernel error (grid_sort_kernel_4)\n");

    ocl->grid_sort_kernels[5] = clCreateKernel(ocl->program, "grid_sort_kernel_5", &err);
    if (err)
        printf("clCreateKernel error (grid_sort_kernel_5)\n");

    ocl->grid_sort_kernels[6] = clCreateKernel(ocl->program, "grid_sort_kernel_6", &err);
    if (err)
        printf("clCreateKernel error (grid_sort_kernel_6)\n");

    ocl->grid_reset_cells = clCreateKernel(ocl->program, "grid_reset_cells", &err);
    if (err)
        printf("clCreateKernel error (grid_reset_cells)\n");

    #endif
}

void ocl_grid_run_sort_kernel(TayState *state, TayGroup *group) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    unsigned long long one = 1;

    /* caculating the space bounding box */
    {
        unsigned long long global_work_size = 1024 * 1;

        {
            err = clSetKernelArg(ocl->grid_sort_kernels[0], 0, sizeof(void *), &group->space.ocl_common.agent_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_0 agent buffer)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[0], 1, sizeof(group->agent_size), &group->agent_size);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_0 agent size)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[0], 2, sizeof(group->space.count), &group->space.count);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_0 agents count)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[0], 3, sizeof(void *), &group->space.ocl_common.space_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_0 space buffer)\n");

            err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernels[0], 1, 0, &global_work_size, 0, 0, 0, 0);
            if (err)
                printf("clEnqueueNDRangeKernel error (grid_sort_kernel_0)\n");
        }

        {
            unsigned boxes_count = (unsigned)global_work_size;

            err = clSetKernelArg(ocl->grid_sort_kernels[1], 0, sizeof(void *), &group->space.ocl_common.space_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_1 space buffer)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[1], 1, sizeof(boxes_count), &boxes_count);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_1 boxes_count)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[1], 2, sizeof(group->space.dims), &group->space.dims);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_1 dims)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[1], 3, sizeof(group->space.min_part_sizes), &group->space.min_part_sizes);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_1 min_part_sizes)\n");

            err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernels[1], 1, 0, &one, 0, 0, 0, 0);
            if (err)
                printf("clEnqueueNDRangeKernel error (grid_sort_kernel_1)\n");
        }
    }

    /* sorting agents into cells */
    {
        {
            err = clSetKernelArg(ocl->grid_sort_kernels[2], 0, sizeof(void *), &group->space.ocl_common.agent_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_2 agent buffer)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[2], 1, sizeof(group->agent_size), &group->agent_size);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_2 agent size)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[2], 2, sizeof(void *), &group->space.ocl_common.space_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_2 space buffer)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[2], 3, sizeof(group->space.dims), &group->space.dims);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_2 dims)\n");

            unsigned long long work_size = group->space.count;

            err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernels[2], 1, 0, &work_size, 0, 0, 0, 0);
            if (err)
                printf("clEnqueueNDRangeKernel error (grid_sort_kernel_2)\n");
        }

        unsigned long long cell_groups = 256;

        {
            err = clSetKernelArg(ocl->grid_sort_kernels[3], 0, sizeof(void *), &group->space.ocl_common.space_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_3 space buffer)\n");

            err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernels[3], 1, 0, &cell_groups, 0, 0, 0, 0);
            if (err)
                printf("clEnqueueNDRangeKernel error (grid_sort_kernel_3)\n");
        }

        {
            err = clSetKernelArg(ocl->grid_sort_kernels[4], 0, sizeof(void *), &group->space.ocl_common.space_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_4 space buffer)\n");

            unsigned cell_groups_4 = (unsigned)cell_groups;

            err = clSetKernelArg(ocl->grid_sort_kernels[4], 1, sizeof(cell_groups_4), &cell_groups_4);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_4 cell groups)\n");

            err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernels[4], 1, 0, &one, 0, 0, 0, 0);
            if (err)
                printf("clEnqueueNDRangeKernel error (grid_sort_kernel_4)\n");
        }

        {
            err = clSetKernelArg(ocl->grid_sort_kernels[5], 0, sizeof(void *), &group->space.ocl_common.space_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_5 space buffer)\n");

            err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernels[5], 1, 0, &cell_groups, 0, 0, 0, 0);
            if (err)
                printf("clEnqueueNDRangeKernel error (grid_sort_kernel_5)\n");
        }
    }

    /* copy agents to the new buffer */
    {
        {
            err = clSetKernelArg(ocl->grid_sort_kernels[6], 0, sizeof(void *), &group->space.ocl_common.agent_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_6 agent buffer)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[6], 1, sizeof(void *), &group->space.ocl_common.agent_sort_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_6 agent sort buffer)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[6], 2, sizeof(group->agent_size), &group->agent_size);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_6 agent size)\n");

            err = clSetKernelArg(ocl->grid_sort_kernels[6], 3, sizeof(void *), &group->space.ocl_common.space_buffer);
            if (err)
                printf("clSetKernelArg error (grid_sort_kernel_6 space buffer)\n");

            unsigned long long work_size = group->space.count;

            err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_sort_kernels[6], 1, 0, &work_size, 0, 0, 0, 0);
            if (err)
                printf("clEnqueueNDRangeKernel error (grid_sort_kernel_6)\n");
        }

        void *temp = group->space.ocl_common.agent_sort_buffer;
        group->space.ocl_common.agent_sort_buffer = group->space.ocl_common.agent_buffer;
        group->space.ocl_common.agent_buffer = temp;
    }

    err = clFinish(ocl->queue);
    if (err)
        printf("clFinish error (grid sort kernels)\n");

#if 0
    char buffer[2024];
    OclGrid *grid = (OclGrid *)buffer;
    err = clEnqueueReadBuffer(ocl->queue, group->space.ocl_common.space_buffer, CL_BLOCKING, 0, 2024, grid, 0, 0, 0);
    if (err)
        printf("clEnqueueReadBuffer error\n");
#endif

    #endif
}

void ocl_grid_run_unsort_kernel(TayState *state, TayGroup *group) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    err = clSetKernelArg(ocl->grid_reset_cells, 0, sizeof(void *), &group->space.ocl_common.space_buffer);
    if (err)
        printf("clSetKernelArg error (grid_reset_cells space buffer)\n");

    unsigned long long work_size = 256;

    err = clEnqueueNDRangeKernel(ocl->queue, ocl->grid_reset_cells, 1, 0, &work_size, 0, 0, 0, 0);
    if (err)
        printf("clEnqueueNDRangeKernel error (grid_reset_cells)\n");

    err = clFinish(ocl->queue);
    if (err)
        printf("clFinish error (grid unsort kernels)\n");

    #endif
}

unsigned ocl_grid_add_see_kernel_text(TayPass *pass, char *text, unsigned remaining_space, int dims) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
kernel void %s(global char *a_agents, global char *b_agents, constant void *c, float4 radii, global OclGrid *seen_grid) {\n\
    unsigned a_i = get_global_id(0);\n\
    const unsigned a_size = %d;\n\
    const unsigned b_size = %d;\n\
    const int dims = %d;\n\
\n\
%s\
\n\
    int4 min_indices = max(convert_int4(floor((box_min - seen_grid->origin) / seen_grid->cell_sizes)), (int4)(0));\n\
    int4 max_indices = min(convert_int4(floor((box_max - seen_grid->origin) / seen_grid->cell_sizes)), seen_grid->cell_counts - 1);\n\
\n\
    int4 indices;\n\
    for (indices.x = min_indices.x; indices.x <= max_indices.x; ++indices.x) {\n\
        for (indices.y = min_indices.y; indices.y <= max_indices.y; ++indices.y) {\n\
            for (indices.z = min_indices.z; indices.z <= max_indices.z; ++indices.z) {\n\
                // unsigned seen_cell_i = grid_z_cell_indices_to_cell_index(indices, dims);\n\
                unsigned seen_cell_i = grid_cell_indices_to_cell_index(indices, seen_grid->cell_counts, dims);\n\
                global OclGridCell *seen_cell = seen_grid->cells + seen_cell_i;\n\
\n\
                unsigned b_beg = seen_cell->offset;\n\
                unsigned b_end = seen_cell->offset + seen_cell->count;\n\
\n\
%s\
            }\n\
        }\n\
    }\n\
}\n\
\n",
    ocl_get_kernel_name(pass),
    pass->seer_group->agent_size,
    pass->seen_group->agent_size,
    dims,
    ocl_get_seer_agent_text(pass),
    ocl_get_coupling_text(pass, dims));

    return length;

    #else
    return 0u;
    #endif
}

void ocl_grid_run_see_kernel(TayOcl *ocl, TayPass *pass) {
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

    err = clSetKernelArg(pass->pass_kernel, 4, sizeof(void *), &pass->seen_group->space.ocl_common.space_buffer);
    if (err)
        printf("clSetKernelArg error (space buffer)\n");

    unsigned long long global_work_size = pass->seer_group->space.count;

    err = clEnqueueNDRangeKernel(ocl->queue, pass->pass_kernel, 1, 0, &global_work_size, 0, 0, 0, 0);
    if (err)
        printf("clEnqueueNDRangeKernel error\n");

    err = clFinish(ocl->queue);
    if (err)
        printf("clFinish error\n");

    #endif
}
