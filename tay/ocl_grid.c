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

void ocl_grid_add_kernel_texts(OclText *text) {
    ocl_text_append(text, "\n\
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
}

int ocl_grid_get_kernels(TayState *state) {
    state->ocl.grid_sort_kernels[0] = ocl_create_kernel(state, "grid_sort_kernel_0");
    if (state->ocl.grid_sort_kernels[0] == 0)
        return 0;
    state->ocl.grid_sort_kernels[1] = ocl_create_kernel(state, "grid_sort_kernel_1");
    if (state->ocl.grid_sort_kernels[1] == 0)
        return 0;
    state->ocl.grid_sort_kernels[2] = ocl_create_kernel(state, "grid_sort_kernel_2");
    if (state->ocl.grid_sort_kernels[2] == 0)
        return 0;
    state->ocl.grid_sort_kernels[3] = ocl_create_kernel(state, "grid_sort_kernel_3");
    if (state->ocl.grid_sort_kernels[3] == 0)
        return 0;
    state->ocl.grid_sort_kernels[4] = ocl_create_kernel(state, "grid_sort_kernel_4");
    if (state->ocl.grid_sort_kernels[4] == 0)
        return 0;
    state->ocl.grid_sort_kernels[5] = ocl_create_kernel(state, "grid_sort_kernel_5");
    if (state->ocl.grid_sort_kernels[5] == 0)
        return 0;
    state->ocl.grid_sort_kernels[6] = ocl_create_kernel(state, "grid_sort_kernel_6");
    if (state->ocl.grid_sort_kernels[6] == 0)
        return 0;
    state->ocl.grid_reset_cells = ocl_create_kernel(state, "grid_reset_cells");
    if (state->ocl.grid_reset_cells == 0)
        return 0;
    return 1;
}

void ocl_grid_run_sort_kernel(TayState *state, TayGroup *group) {
    unsigned one = 1;

    /* caculating the space bounding box */
    {
        unsigned global_work_size = 1024 * 1;

        {
            void *kernel = state->ocl.grid_sort_kernels[0];

            if (!ocl_set_buffer_kernel_arg(state, 0, kernel, &group->space.ocl_common.agent_buffer))
                return;
            if (!ocl_set_value_kernel_arg(state, 1, kernel, &group->agent_size, sizeof(group->agent_size)))
                return;
            if (!ocl_set_value_kernel_arg(state, 2, kernel, &group->space.count, sizeof(group->space.count)))
                return;
            if (!ocl_set_buffer_kernel_arg(state, 3, kernel, &group->space.ocl_common.space_buffer))
                return;
            if (!ocl_run_kernel(state, kernel, global_work_size, 0))
                return;
        }

        {
            void *kernel = state->ocl.grid_sort_kernels[1];
            unsigned boxes_count = global_work_size;

            if (!ocl_set_buffer_kernel_arg(state, 0, kernel, &group->space.ocl_common.space_buffer))
                return;
            if (!ocl_set_value_kernel_arg(state, 1, kernel, &boxes_count, sizeof(boxes_count)))
                return;
            if (!ocl_set_value_kernel_arg(state, 2, kernel, &group->space.dims, sizeof(group->space.dims)))
                return;
            if (!ocl_set_value_kernel_arg(state, 3, kernel, &group->space.min_part_sizes, sizeof(group->space.min_part_sizes)))
                return;
            if (!ocl_run_kernel(state, kernel, one, 0))
                return;
        }
    }

    /* sorting agents into cells */
    {
        {
            void *kernel = state->ocl.grid_sort_kernels[2];
            unsigned work_size = group->space.count;

            if (!ocl_set_buffer_kernel_arg(state, 0, kernel, &group->space.ocl_common.agent_buffer))
                return;
            if (!ocl_set_value_kernel_arg(state, 1, kernel, &group->agent_size, sizeof(group->agent_size)))
                return;
            if (!ocl_set_buffer_kernel_arg(state, 2, kernel, &group->space.ocl_common.space_buffer))
                return;
            if (!ocl_set_value_kernel_arg(state, 3, kernel, &group->space.dims, sizeof(group->space.dims)))
                return;
            if (!ocl_run_kernel(state, kernel, work_size, 0))
                return;
        }

        unsigned cell_groups = 256;

        {
            void *kernel = state->ocl.grid_sort_kernels[3];

            if (!ocl_set_buffer_kernel_arg(state, 0, kernel, &group->space.ocl_common.space_buffer))
                return;
            if (!ocl_run_kernel(state, kernel, cell_groups, 0))
                return;
        }

        {
            void *kernel = state->ocl.grid_sort_kernels[4];

            if (!ocl_set_buffer_kernel_arg(state, 0, kernel, &group->space.ocl_common.space_buffer))
                return;
            if (!ocl_set_value_kernel_arg(state, 1, kernel, &cell_groups, sizeof(cell_groups)))
                return;
            if (!ocl_run_kernel(state, kernel, one, 0))
                return;
        }

        {
            void *kernel = state->ocl.grid_sort_kernels[5];

            if (!ocl_set_buffer_kernel_arg(state, 0, kernel, &group->space.ocl_common.space_buffer))
                return;
            if (!ocl_run_kernel(state, kernel, cell_groups, 0))
                return;
        }
    }

    /* copy agents to the new buffer */
    {
        {
            void *kernel = state->ocl.grid_sort_kernels[6];
            unsigned work_size = group->space.count;

            if (!ocl_set_buffer_kernel_arg(state, 0, kernel, &group->space.ocl_common.agent_buffer))
                return;
            if (!ocl_set_buffer_kernel_arg(state, 1, kernel, &group->space.ocl_common.agent_sort_buffer))
                return;
            if (!ocl_set_value_kernel_arg(state, 2, kernel, &group->agent_size, sizeof(group->agent_size)))
                return;
            if (!ocl_set_buffer_kernel_arg(state, 3, kernel, &group->space.ocl_common.space_buffer))
                return;
            if (!ocl_run_kernel(state, kernel, work_size, 0))
                return;
        }

        void *temp = group->space.ocl_common.agent_sort_buffer;
        group->space.ocl_common.agent_sort_buffer = group->space.ocl_common.agent_buffer;
        group->space.ocl_common.agent_buffer = temp;
    }

    ocl_finish(state);

#if 0
    char buffer[2024];
    OclGrid *grid = (OclGrid *)buffer;
    err = clEnqueueReadBuffer(ocl->queue, group->space.ocl_common.space_buffer, CL_BLOCKING, 0, 2024, grid, 0, 0, 0);
    if (err)
        printf("clEnqueueReadBuffer error\n");
#endif
}

void ocl_grid_run_unsort_kernel(TayState *state, TayGroup *group) {
    void *kernel = state->ocl.grid_reset_cells;
    unsigned work_size = 256;

    if (!ocl_set_buffer_kernel_arg(state, 0, kernel, &group->space.ocl_common.space_buffer))
        return;
    if (!ocl_run_kernel(state, kernel, work_size, 0))
        return;

    ocl_finish(state);
}

void ocl_grid_add_seen_text(OclText *text, TayPass *pass, int dims) {

    ocl_text_append(text, "{ /* grid seen loop */\n\
    global OclGrid *seen_grid = space_buffer;\n\
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
} /* grid seen loop */\n", ocl_get_coupling_text(pass, dims));
}

void ocl_grid_add_see_kernel_text(OclText *text, TayPass *pass, int dims) {

    ocl_text_append(text, "\n\
kernel void %s(global char *a_agents, global char *b_agents, constant void *c, float4 radii, global void *space_buffer) {\n\
    unsigned a_i = get_global_id(0);\n\
    const unsigned a_size = %d;\n\
    const unsigned b_size = %d;\n\
    const int dims = %d;\n\
\n\
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
