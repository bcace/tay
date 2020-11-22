#include "space.h"
#include "state.h"
#include "gpu.h"
#include <stdio.h>
#include <assert.h>

// TODO: can we merge this with something else?
#define GPU_TREE_NULL_INDEX     -1


static const char *HEADER = "\n\
#define TAY_MAX_GROUPS %d\n\
#define GPU_TREE_NULL_INDEX %d\n\
\n\
\n\
typedef struct __attribute__((packed)) {\n\
    float4 min;\n\
    float4 max;\n\
} Box;\n\
\n\
typedef struct __attribute__((packed)) Cell {\n\
    union {\n\
        global struct Cell *lo;\n\
        unsigned long lo_index;\n\
    };\n\
    union {\n\
        global struct Cell *hi;\n\
        unsigned long hi_index;\n\
    };\n\
    global TayAgentTag *first[TAY_MAX_GROUPS];\n\
    Box box;\n\
} Cell;\n\
\n\
kernel void resolve_cell_pointers(global Cell *cells) {\n\
    int i = get_global_id(0);\n\
    global Cell *cell = cells + i;\n\
    cell->lo = (cell->lo_index != GPU_TREE_NULL_INDEX) ? cells + cell->lo_index : 0;\n\
    cell->hi = (cell->hi_index != GPU_TREE_NULL_INDEX) ? cells + cell->hi_index : 0;\n\
}\n\
\n\
kernel void resolve_cell_agent_pointers(global Cell *cells, global char *agents, int agent_size, int group_index, unsigned long host_agents_base) {\n\
    int i = get_global_id(0);\n\
    global Cell *cell = cells + i;\n\
    if (cell->first[group_index])\n\
        cell->first[group_index] = (global TayAgentTag *)(agents + ((unsigned long)cell->first[group_index] - host_agents_base));\n\
}\n";

static const char *SEE_KERNEL = "\n\
kernel void %s_tree_kernel(global char *agents, int agent_size, global Cell *cells, int seen_group, float4 radii, global void *see_context) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *seer_agent = (global TayAgentTag *)(agents + i * agent_size);\n\
    float4 seer_p = *(global float4 *)(agents + i * agent_size + sizeof(TayAgentTag));\n\
\n\
    Box seer_box;\n\
    seer_box.min = seer_p - radii;\n\
    seer_box.max = seer_p + radii;\n\
\n\
    global Cell *stack[16];\n\
    int stack_size = 0;\n\
    stack[stack_size++] = cells;\n\
\n\
    while (stack_size) {\n\
        global Cell *seen_cell = stack[--stack_size];\n\
\n\
        while (seen_cell) {\n\
\n\
            for (int j = 0; j < DIMS; ++j) {\n\
                if (seer_box.min[j] > seen_cell->box.max[j] || seer_box.max[j] < seen_cell->box.min[j]) {\n\
                    seen_cell = 0;\n\
                    goto SKIP_CELL;\n\
                }\n\
            }\n\
\n\
            if (seen_cell->hi)\n\
                stack[stack_size++] = seen_cell->hi;\n\
\n\
            global TayAgentTag *seen_agent = seen_cell->first[seen_group];\n\
            while (seen_agent) {\n\
                float4 seen_p = TAY_AGENT_POSITION(seen_agent);\n\
\n\
                if (seer_agent == seen_agent)\n\
                    goto SKIP_SEE;\n\
\n\
                float4 d = seer_p - seen_p;\n\
                for (int j = 0; j < DIMS; ++j) {\n\
                    if (d[j] > radii[j] || d[j] < -radii[j])\n\
                        goto SKIP_SEE;\n\
                }\n\
\n\
                %s((global void *)seer_agent, (global void *)seen_agent, see_context);\n\
\n\
                SKIP_SEE:;\n\
                seen_agent = seen_agent->next;\n\
            }\n\
\n\
            seen_cell = seen_cell->lo;\n\
\n\
            SKIP_CELL:;\n\
        }\n\
    }\n\
}\n";

static const char *ACT_KERNEL = "\n\
kernel void %s_tree_kernel(global char *agents, int agent_size, global void *act_context) {\n\
    %s((global void *)(agents + get_global_id(0) * agent_size), act_context);\n\
}\n";

void gpu_tree_add_source(TayState *state) {
    GpuShared *shared = &state->space.gpu_shared;

    shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, HEADER,
                                   TAY_MAX_GROUPS, GPU_TREE_NULL_INDEX);

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, SEE_KERNEL,
                                           pass->func_name, pass->func_name);
        else if (pass->type == TAY_PASS_ACT)
            shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, ACT_KERNEL,
                                           pass->func_name, pass->func_name);
        else
            assert(0); /* unhandled pass type */
    }
}

#pragma pack(push, 1)
typedef struct {
    unsigned long long lo_index;
    unsigned long long hi_index;
    void *first[TAY_MAX_GROUPS];
    Box box;
} GpuTreeCell;
#pragma pack(pop)

void gpu_tree_on_simulation_start(TayState *state) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;
    GpuTree *tree = &space->gpu_tree;

    /* pass kernels */
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;

        static char kernel_name[512];
        sprintf_s(kernel_name, 512, "%s_tree_kernel", pass->func_name);

        GpuKernel kernel = 0;

        if (pass->type == TAY_PASS_SEE) {
            kernel = gpu_create_kernel(shared->gpu, kernel_name);
            TayGroup *seer_group = state->groups + pass->seer_group;
            gpu_set_kernel_buffer_argument(kernel, 0, &shared->agent_buffers[pass->seer_group]);
            gpu_set_kernel_value_argument(kernel, 1, &seer_group->agent_size, sizeof(seer_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &shared->cells_buffer);
            gpu_set_kernel_value_argument(kernel, 3, &pass->seen_group, sizeof(pass->seen_group));
            gpu_set_kernel_value_argument(kernel, 4, &pass->radii, sizeof(pass->radii));
            gpu_set_kernel_buffer_argument(kernel, 5, &shared->pass_context_buffers[i]);
        }
        else if (pass->type == TAY_PASS_ACT) {
            kernel = gpu_create_kernel(shared->gpu, kernel_name);
            TayGroup *act_group = state->groups + pass->act_group;
            gpu_set_kernel_buffer_argument(kernel, 0, &shared->agent_buffers[pass->act_group]);
            gpu_set_kernel_value_argument(kernel, 1, &act_group->agent_size, sizeof(act_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &shared->pass_context_buffers[i]);
        }
        else
            assert(0); /* unhandled pass type */

        tree->pass_kernels[i] = kernel;
    }

    /* private kernels */
    {
        tree->resolve_cell_pointers_kernel = gpu_create_kernel(shared->gpu, "resolve_cell_pointers");
        gpu_set_kernel_buffer_argument(tree->resolve_cell_pointers_kernel, 0, &shared->cells_buffer);

        tree->resolve_cell_agent_pointers_kernel = gpu_create_kernel(shared->gpu, "resolve_cell_agent_pointers");
        gpu_set_kernel_buffer_argument(tree->resolve_cell_agent_pointers_kernel, 0, &shared->cells_buffer);
    }
}


void gpu_tree_on_simulation_end(TayState *state) {
    GpuTree *tree = &state->space.gpu_tree;
    for (int i = 0; i < state->passes_count; ++i)
        gpu_release_kernel(tree->pass_kernels[i]);
}

static void _push_cells(TayState *state) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;
    GpuTree *tree = &space->gpu_tree;
    Tree *base = &tree->base;

    /* send cells to GPU */
    {
        GpuTreeCell *gpu_cells = space_get_temp_arena(space, base->cells_count * sizeof(GpuTreeCell));

        for (int i = 0; i < base->cells_count; ++i) {
            TreeCell *cell = base->cells + i;
            GpuTreeCell *gpu_cell = gpu_cells + i;

            gpu_cell->lo_index = cell->lo ? cell->lo - base->cells : GPU_TREE_NULL_INDEX;
            gpu_cell->hi_index = cell->hi ? cell->hi - base->cells : GPU_TREE_NULL_INDEX;
            gpu_cell->box = cell->box;

            for (int j = 0; j < TAY_MAX_GROUPS; ++j)
                gpu_cell->first[j] = cell->first[j];
        }

        gpu_enqueue_write_buffer(shared->gpu, shared->cells_buffer, GPU_BLOCKING, base->cells_count * sizeof(GpuTreeCell), gpu_cells);
    }

    /* fix cell-cell and cell-agent pointers on GPU */
    {
        long long int cells_count = base->cells_count;

        gpu_enqueue_kernel_nb(shared->gpu, tree->resolve_cell_pointers_kernel, &cells_count);

        for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
            TayGroup *group = state->groups + i;
            if (group->storage) {
                unsigned long long host_agents_base = (unsigned long long)group->storage;
                gpu_set_kernel_buffer_argument(tree->resolve_cell_agent_pointers_kernel, 1, &shared->agent_buffers[i]);
                gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 2, &group->agent_size, sizeof(group->agent_size));
                gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 3, &i, sizeof(i));
                gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 4, &host_agents_base, sizeof(host_agents_base));
                gpu_enqueue_kernel_nb(shared->gpu, tree->resolve_cell_agent_pointers_kernel, &cells_count);
                gpu_finish(shared->gpu);
            }
        }

    }
}

static void _see(TayState *state, int pass_index) {
    GpuShared *shared = &state->space.gpu_shared;
    GpuTree *tree = &state->space.gpu_tree;
    TayPass *pass = state->passes + pass_index;
    TayGroup *group = state->groups + pass->seer_group;
    gpu_enqueue_kernel(shared->gpu, tree->pass_kernels[pass_index], group->capacity);
}

static void _act(TayState *state, int pass_index) {
    GpuShared *shared = &state->space.gpu_shared;
    GpuTree *tree = &state->space.gpu_tree;
    TayPass *pass = state->passes + pass_index;
    TayGroup *group = state->groups + pass->act_group;
    gpu_enqueue_kernel(shared->gpu, tree->pass_kernels[pass_index], group->capacity);
}

void gpu_tree_step(TayState *state) {
    tree_update(&state->space);
    _push_cells(state);
    space_gpu_shared_fix_gpu_pointers(state);

    /* do passes */
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            _see(state, i);
        else if (pass->type == TAY_PASS_ACT)
            _act(state, i);
        else
            assert(0); /* unhandled pass type */
    }

    tree_return_agents(&state->space);
}
