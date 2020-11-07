#include "tree.h"
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define GPU_TREE_NULL_INDEX -1
#define GPU_TREE_MAX_TEXT_SIZE 10000
#define GPU_TREE_ARENA_SIZE (TAY_MAX_AGENTS * sizeof(float4))


const char *GPU_TREE_HEADER = "\n\
#define DIMS %d\n\
#define TAY_MAX_GROUPS %d\n\
#define TAY_MAX_DIMENSIONS %d\n\
#define GPU_TREE_NULL_INDEX %d\n\
\n\
#define AGENT_POSITION_PTR(__agent_tag__) ((global float4 *)(__agent_tag__ + 1))\n\
\n\
\n\
typedef struct __attribute__((packed)) TayAgentTag {\n\
    global struct TayAgentTag *next;\n\
} TayAgentTag;\n\
\n\
typedef struct __attribute__((packed)) {\n\
    float min[TAY_MAX_DIMENSIONS];\n\
    float max[TAY_MAX_DIMENSIONS];\n\
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
}\n\
\n\
kernel void resolve_agent_pointers(global TayAgentTag *tags, global char *agents, int agent_size, unsigned long host_agents_base) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *tag = tags + i;\n\
    global TayAgentTag *agent_tag = (global TayAgentTag *)(agents + i * agent_size);\n\
    if (tag->next)\n\
        agent_tag->next = (global TayAgentTag *)(agents + ((unsigned long)tag->next - host_agents_base));\n\
    else\n\
        agent_tag->next = 0;\n\
}\n\
\n\
kernel void fetch_agent_positions(global char *agents, global float4 *positions, int agent_size) {\n\
    int i = get_global_id(0);\n\
    positions[i] = *(global float4 *)(agents + i * agent_size + sizeof(TayAgentTag));\n\
}\n";

/* NOTE: I can have several versions of this kernel, one that iterates through all agents of a seer cell from a single thread,
and another one that puts each seer agent in its own thread. Iterating through seen agents is always the same. Maybe
with minor differences, in first case I can just test box overlaps between seer and seen cells, and in both cases I can
test box overlaps between seer agent's box and seen cell's box. */

const char *GPU_TREE_SEE_KERNEL = "\n\
kernel void %s_kernel(global char *agents, int agent_size, global Cell *cells, int seen_group, float4 radii, global void *see_context) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *seer_agent = (global TayAgentTag *)(agents + i * agent_size);\n\
    float4 seer_p = *(global float4 *)(agents + i * agent_size + sizeof(TayAgentTag));\n\
\n\
    Box seer_box;\n\
    seer_box.min[0] = seer_p.x - radii.x;\n\
    seer_box.max[0] = seer_p.x + radii.x;\n\
    seer_box.min[1] = seer_p.y - radii.y;\n\
    seer_box.max[1] = seer_p.y + radii.y;\n\
    seer_box.min[2] = seer_p.z - radii.z;\n\
    seer_box.max[2] = seer_p.z + radii.z;\n\
    seer_box.min[3] = seer_p.w - radii.w;\n\
    seer_box.max[3] = seer_p.w + radii.w;\n\
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
                float4 seen_p = *AGENT_POSITION_PTR(seen_agent);\n\
\n\
                if (seer_agent == seen_agent)\n\
                    goto SKIP_SEE;\n\
\n\
                float4 d = seer_p - seen_p;\n\
                if (d.x > radii.x || d.x < -radii.x)\n\
                    goto SKIP_SEE;\n\
#if DIMS > 1\n\
                if (d.y > radii.y || d.y < -radii.y)\n\
                    goto SKIP_SEE;\n\
#endif\n\
#if DIMS > 2\n\
                if (d.z > radii.z || d.z < -radii.z)\n\
                    goto SKIP_SEE;\n\
#endif\n\
#if DIMS > 3\n\
                if (d.w > radii.w || d.w < -radii.w)\n\
                    goto SKIP_SEE;\n\
#endif\n\
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

const char *GPU_TREE_ACT_KERNEL = "\n\
kernel void %s_kernel(global Cell *cells, int act_group, global void *act_context) {\n\
    int i = get_global_id(0);\n\
    global Cell *cell = cells + i;\n\
    global TayAgentTag *agent = cell->first[act_group];\n\
    while (agent) {\n\
        %s((global void *)agent, act_context);\n\
        agent = agent->next;\n\
    }\n\
}\n";

// #pragma pack(push, 1)
// typedef struct {
//     int lo, hi;
//     int first[TAY_MAX_GROUPS];
//     Box box;
// } CellBridge;
// #pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    unsigned long long lo_index;
    unsigned long long hi_index;
    void *first[TAY_MAX_GROUPS];
    Box box;
} GPUCell;
#pragma pack(pop)

typedef struct {
    TreeBase base;
    GpuContext *gpu;
    GpuBuffer agent_buffers[TAY_MAX_GROUPS];
    GpuBuffer pass_context_buffers[TAY_MAX_PASSES];
    // GpuBuffer bridges_buffer;
    GpuBuffer cells_buffer;
    GpuBuffer agent_io_buffer; /* general buffer for whatever per-agent data */
    GpuKernel resolve_cell_pointers_kernel;
    GpuKernel resolve_cell_agent_pointers_kernel;
    GpuKernel resolve_agent_pointers_kernel;
    GpuKernel fetch_agent_positions_kernel;
    GpuKernel pass_kernels[TAY_MAX_PASSES];
    char text[GPU_TREE_MAX_TEXT_SIZE];
    int text_size;
    char arena[GPU_TREE_ARENA_SIZE];
} Tree;

static Tree *_init(int dims, float *radii, int max_depth_correction) {
    Tree *tree = malloc(sizeof(Tree));
    tree_init(&tree->base, dims, radii, max_depth_correction);
    tree->gpu = gpu_create();
    tree->text_size = 0;
    return tree;
}

static void _destroy(TaySpaceContainer *container) {
    Tree *tree = container->storage;
    gpu_destroy(tree->gpu);
    tree_destroy(&tree->base);
}

static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
    tree_add(container->storage, agent, group);
}

static void _on_simulation_start(TayState *state) {
    Tree *tree = state->space.storage;

    /* create buffers */

    // tree->bridges_buffer = gpu_create_buffer(tree->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, tree->base.max_cells * sizeof(CellBridge));
    tree->cells_buffer = gpu_create_buffer(tree->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, tree->base.max_cells * sizeof(GPUCell));
    tree->agent_io_buffer = gpu_create_buffer(tree->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, GPU_TREE_ARENA_SIZE);

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            tree->agent_buffers[i] = gpu_create_buffer(tree->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, group->capacity * group->agent_size);
    }

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        tree->pass_context_buffers[i] = gpu_create_buffer(tree->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, pass->context_size);
    }

    /* compose kernels source text */

    tree->text_size = 0;
    tree->text_size += sprintf_s(tree->text + tree->text_size, GPU_TREE_MAX_TEXT_SIZE - tree->text_size, GPU_TREE_HEADER,
                                 state->space.dims, TAY_MAX_GROUPS, TAY_MAX_DIMENSIONS, GPU_TREE_NULL_INDEX);

    tree->text_size += sprintf_s(tree->text + tree->text_size, GPU_TREE_MAX_TEXT_SIZE - tree->text_size, state->source);

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            tree->text_size += sprintf_s(tree->text + tree->text_size, GPU_TREE_MAX_TEXT_SIZE - tree->text_size, GPU_TREE_SEE_KERNEL,
                                         pass->func_name, pass->func_name);
        else if (pass->type == TAY_PASS_ACT)
            tree->text_size += sprintf_s(tree->text + tree->text_size, GPU_TREE_MAX_TEXT_SIZE - tree->text_size, GPU_TREE_ACT_KERNEL,
                                         pass->func_name, pass->func_name);
        else
            assert(0); /* unhandled pass type */
    }

    /* build program */

    gpu_build_program(tree->gpu, tree->text);

    /* create kernels */

    // global Cell *cells, unsigned long long host_cells_base
    tree->resolve_cell_pointers_kernel = gpu_create_kernel(tree->gpu, "resolve_cell_pointers");
    gpu_set_kernel_buffer_argument(tree->resolve_cell_pointers_kernel, 0, &tree->cells_buffer);
    // unsigned long long host_cells_base = (unsigned long long)tree->base.cells;
    // gpu_set_kernel_value_argument(tree->resolve_cell_pointers_kernel, 1, &host_cells_base, sizeof(host_cells_base));

    // global Cell *cells, global char *agents, int agent_size, int group_index, unsigned long long host_agents_base
    tree->resolve_cell_agent_pointers_kernel = gpu_create_kernel(tree->gpu, "resolve_cell_agent_pointers");
    gpu_set_kernel_buffer_argument(tree->resolve_cell_agent_pointers_kernel, 0, &tree->cells_buffer);

    // global TayAgentTag *tags, global char *agents, int agent_size, unsigned long long host_agents_base
    tree->resolve_agent_pointers_kernel = gpu_create_kernel(tree->gpu, "resolve_agent_pointers");
    gpu_set_kernel_buffer_argument(tree->resolve_agent_pointers_kernel, 0, &tree->agent_io_buffer);

    tree->fetch_agent_positions_kernel = gpu_create_kernel(tree->gpu, "fetch_agent_positions");
    gpu_set_kernel_buffer_argument(tree->fetch_agent_positions_kernel, 1, &tree->agent_io_buffer);

    /* pass kernels */

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;

        static char kernel_name[512];
        sprintf_s(kernel_name, 512, "%s_kernel", pass->func_name);

        GpuKernel kernel = 0;

        float4 radii; /* fix this sizeof once we switch to float4 in TayPass */
        radii.x = pass->radii[0];
        radii.y = pass->radii[1];
        radii.z = pass->radii[2];
        radii.w = pass->radii[3];

        if (pass->type == TAY_PASS_SEE) {
            kernel = gpu_create_kernel(tree->gpu, kernel_name);
            TayGroup *seer_group = state->groups + pass->seer_group;
            gpu_set_kernel_buffer_argument(kernel, 0, &tree->agent_buffers[pass->seer_group]);
            gpu_set_kernel_value_argument(kernel, 1, &seer_group->agent_size, sizeof(seer_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &tree->cells_buffer);
            gpu_set_kernel_value_argument(kernel, 3, &pass->seen_group, sizeof(pass->seen_group));
            gpu_set_kernel_value_argument(kernel, 4, &radii, sizeof(radii));
            gpu_set_kernel_buffer_argument(kernel, 5, &tree->pass_context_buffers[i]);
        }
        else if (pass->type == TAY_PASS_ACT) {
            kernel = gpu_create_kernel(tree->gpu, kernel_name);
            gpu_set_kernel_buffer_argument(kernel, 0, &tree->cells_buffer);
            gpu_set_kernel_value_argument(kernel, 1, &pass->act_group, sizeof(pass->act_group));
            gpu_set_kernel_buffer_argument(kernel, 2, &tree->pass_context_buffers[i]);
        }
        else
            assert(0); /* unhandled pass type */

        tree->pass_kernels[i] = kernel;
    }

    /* push agents to GPU */

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            gpu_enqueue_write_buffer(tree->gpu, tree->agent_buffers[i], GPU_BLOCKING, group->capacity * group->agent_size, group->storage);
    }

    /* push pass contexts to GPU */

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        gpu_enqueue_write_buffer(tree->gpu, tree->pass_context_buffers[i], GPU_BLOCKING, pass->context_size, pass->context);
    }
}

static void _on_simulation_end(TayState *state) {
    Tree *tree = state->space.storage;
    gpu_release_kernel(tree->resolve_cell_pointers_kernel);
    gpu_release_kernel(tree->resolve_cell_agent_pointers_kernel);
    gpu_release_kernel(tree->resolve_agent_pointers_kernel);
}

static void _on_step_start(TayState *state) {
    Tree *tree = state->space.storage;
    TreeBase *base = &tree->base;

    tree_update(base);

    GPUCell *gpu_cells = (GPUCell *)tree->arena;
    assert(tree->base.cells_count * sizeof(GPUCell) < GPU_TREE_ARENA_SIZE);

    for (int i = 0; i < base->cells_count; ++i) {
        Cell *cell = base->cells + i;
        GPUCell *gpu_cell = gpu_cells + i;

        gpu_cell->lo_index = cell->lo ? cell->lo - base->cells : GPU_TREE_NULL_INDEX;
        gpu_cell->hi_index = cell->hi ? cell->hi - base->cells : GPU_TREE_NULL_INDEX;
        gpu_cell->box = cell->box;

        for (int j = 0; j < TAY_MAX_GROUPS; ++j)
            gpu_cell->first[j] = cell->first[j];
    }

    // CellBridge *bridges = (CellBridge *)tree->arena;
    // assert(tree->base.cells_count * sizeof(CellBridge) < GPU_TREE_ARENA_SIZE);

    // /* translate cells into cell bridges */

    // for (int i = 0; i < base->cells_count; ++i) {
    //     Cell *cell = base->cells + i;
    //     CellBridge *bridge = bridges + i;

    //     bridge->lo = (cell->lo != 0) ? (int)(cell->lo - base->cells) : GPU_TREE_NULL_INDEX;
    //     bridge->hi = (cell->hi != 0) ? (int)(cell->hi - base->cells) : GPU_TREE_NULL_INDEX;
    //     bridge->box = cell->box;

    //     for (int j = 0; j < TAY_MAX_GROUPS; ++j)
    //         bridge->first[j] = group_tag_to_index(state->groups + j, cell->first[j]);
    // }

    /* send bridges to GPU */

    gpu_enqueue_write_buffer(tree->gpu, tree->cells_buffer, GPU_BLOCKING, base->cells_count * sizeof(GPUCell), gpu_cells);

    /* resolve pointers on GPU side and copy bounding boxes */

    long long int cells_count = base->cells_count;
    gpu_enqueue_kernel_nb(tree->gpu, tree->resolve_cell_pointers_kernel, &cells_count);

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {

            unsigned long long host_agents_base = (unsigned long long)group->storage;

            /* resolve cell agent pointers */

            // global Cell *cells, global char *agents, int agent_size, int group_index, unsigned long long host_agents_base

            gpu_set_kernel_buffer_argument(tree->resolve_cell_agent_pointers_kernel, 1, &tree->agent_buffers[i]);
            gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 2, &group->agent_size, sizeof(group->agent_size));
            gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 3, &i, sizeof(i));
            gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 4, &host_agents_base, sizeof(host_agents_base));

            long long int cells_count = base->cells_count;
            gpu_enqueue_kernel_nb(tree->gpu, tree->resolve_cell_agent_pointers_kernel, &cells_count);

            /* resolve agent next pointers */

            // global TayAgentTag *tags, global char *agents, int agent_size, unsigned long long host_agents_base

            TayAgentTag *tags = (TayAgentTag *)tree->arena;
            assert(group->capacity * sizeof(TayAgentTag) < GPU_TREE_ARENA_SIZE);

            for (int j = 0; j < group->capacity; ++j)
                tags[j] = *(TayAgentTag *)((char *)group->storage + j * group->agent_size);

            gpu_enqueue_write_buffer(tree->gpu, tree->agent_io_buffer, GPU_BLOCKING, group->capacity * sizeof(TayAgentTag), tags);

            gpu_set_kernel_buffer_argument(tree->resolve_agent_pointers_kernel, 1, &tree->agent_buffers[i]);
            gpu_set_kernel_value_argument(tree->resolve_agent_pointers_kernel, 2, &group->agent_size, sizeof(group->agent_size));
            gpu_set_kernel_value_argument(tree->resolve_agent_pointers_kernel, 3, &host_agents_base, sizeof(host_agents_base));

            long long int group_capacity = group->capacity;
            gpu_enqueue_kernel_nb(tree->gpu, tree->resolve_agent_pointers_kernel, &group_capacity);

            gpu_finish(tree->gpu);
        }
    }
}

static void _on_step_end(TayState *state) {
    Tree *tree = state->space.storage;

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;

        if (group->storage) {
            int req_size = group->capacity * sizeof(float4) * (group->is_point ? 1 : 2);
            assert(req_size < GPU_TREE_ARENA_SIZE);

            /* copy all agent positions into a buffer */

            gpu_set_kernel_buffer_argument(tree->fetch_agent_positions_kernel, 0, &tree->agent_buffers[i]);
            gpu_set_kernel_value_argument(tree->fetch_agent_positions_kernel, 2, &group->agent_size, sizeof(group->agent_size));

            long long int group_capacity = group->capacity;
            gpu_enqueue_kernel_nb(tree->gpu, tree->fetch_agent_positions_kernel, &group_capacity);

            gpu_finish(tree->gpu);

            /* copy the buffer into arena */

            gpu_enqueue_read_buffer(tree->gpu, tree->agent_io_buffer, GPU_BLOCKING, req_size, tree->arena);

            /* copy positions from arena into agents */

            float4 *positions = (float4 *)tree->arena;
            for (int i = 0; i < group->capacity; ++i) {
                *(float4 *)((char *)group->storage + group->agent_size * i + sizeof(TayAgentTag)) = positions[i];
            }
        }
    }
}

static void _see(TayState *state, int pass_index) {
    Tree *tree = state->space.storage;
    TayPass *pass = state->passes + pass_index;
    TayGroup *group = state->groups + pass->seer_group;
    gpu_enqueue_kernel(tree->gpu, tree->pass_kernels[pass_index], group->capacity);
}

static void _act(TayState *state, int pass_index) {
    Tree *tree = state->space.storage;
    gpu_enqueue_kernel(tree->gpu, tree->pass_kernels[pass_index], tree->base.cells_count);
}

static void _on_run_end(TaySpaceContainer *container, TayState *state) {
    Tree *tree = (Tree *)container->storage;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {
            gpu_enqueue_read_buffer(tree->gpu, tree->agent_buffers[i], GPU_BLOCKING, group->capacity * group->agent_size, group->storage);
            // TODO: cannot copy "next" pointer!!!
        }
    }
}

static void _null(TayState *state, int pass_index) {}

void space_gpu_tree_init(TaySpaceContainer *container, int dims, float *radii, int max_depth_correction) {
    assert(sizeof(unsigned long long) == sizeof(void *));
    space_container_init(container, _init(dims, radii, max_depth_correction), dims, _destroy);
    container->on_simulation_start = _on_simulation_start;
    container->on_simulation_end = _on_simulation_end;
    container->on_step_start = _on_step_start;
    container->on_step_end = _on_step_end;
    container->add = _add;
    container->see = _see;
    container->act = _act;
    container->on_run_end = _on_run_end;
}
