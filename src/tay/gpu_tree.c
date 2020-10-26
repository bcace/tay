#include "tree.h"
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define GPU_TREE_NULL_INDEX -1
#define GPU_TREE_MAX_TEXT_SIZE 10000
#define GPU_TREE_ARENA_SIZE (TAY_MAX_AGENTS * sizeof(int))


const char *HEADER = "\n\
#define TAY_MAX_GROUPS %d\n\
#define TAY_MAX_DIMENSIONS %d\n\
#define GPU_TREE_NULL_INDEX %d\n\
\n\
typedef struct __attribute__((packed)) TayAgentTag {\n\
    global struct TayAgentTag *next;\n\
} TayAgentTag;\n\
\n\
\n\
typedef struct __attribute__((packed)) {\n\
    float min[TAY_MAX_DIMENSIONS];\n\
    float max[TAY_MAX_DIMENSIONS];\n\
} Box;\n\
\n\
typedef struct __attribute__((packed)) {\n\
    int lo, hi;\n\
    int first[TAY_MAX_GROUPS];\n\
    Box box;\n\
} CellBridge;\n\
\n\
typedef struct __attribute__((packed)) Cell {\n\
    global struct Cell *lo, *hi;\n\
    global TayAgentTag *first[TAY_MAX_GROUPS];\n\
    Box box;\n\
} Cell;\n\
\n\
kernel void resolve_cell_pointers(global CellBridge *bridges, global Cell *cells) {\n\
    int i = get_global_id(0);\n\
    cells[i].lo = (bridges[i].lo != GPU_TREE_NULL_INDEX) ? (cells + bridges[i].lo) : 0;\n\
    cells[i].hi = (bridges[i].hi != GPU_TREE_NULL_INDEX) ? (cells + bridges[i].hi) : 0;\n\
    cells[i].box = bridges[i].box;\n\
}\n\
\n\
kernel void resolve_cell_agent_pointers(global CellBridge *bridges, global Cell *cells, int cells_count, global char *agents, int agent_size, int group_index) {\n\
    int i = get_global_id(0);\n\
    cells[i].first[group_index] = (bridges[i].first[group_index] != GPU_TREE_NULL_INDEX) ? (global TayAgentTag *)(agents + bridges[i].first[group_index] * agent_size) : 0;\n\
}\n\
\n\
kernel void resolve_agent_pointers(global char *agents, global int *next_indices, int agent_size) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *tag = (global TayAgentTag *)(agents + i * agent_size);\n\
    if (next_indices[i] != GPU_TREE_NULL_INDEX)\n\
        tag->next = (global TayAgentTag *)(agents + next_indices[i] * agent_size);\n\
    else\n\
        tag->next = 0;\n\
}\n";

#pragma pack(push, 1)
typedef struct {
    int lo, hi;
    int first[TAY_MAX_GROUPS];
    Box box;
} CellBridge;
#pragma pack(pop)

/* NOTE: this is here just to be able to create a buffer of correct size on the GPU,
but this strut is not actually used in host. Should be the same size as the Cell
struct defined in the kernels source. */
#pragma pack(push, 1)
typedef struct {
    void *lo, *hi;
    void *first[TAY_MAX_GROUPS];
    Box box;
} GPUCell;
#pragma pack(pop)

typedef struct {
    TreeBase base;
    GpuContext *gpu;
    GpuBuffer agent_buffers[TAY_MAX_GROUPS];
    GpuBuffer bridges_buffer;
    GpuBuffer cells_buffer;
    GpuBuffer next_indices_buffer;
    GpuKernel resolve_cell_pointers_kernel;
    GpuKernel resolve_cell_agent_pointers_kernel;
    GpuKernel resolve_agent_pointers_kernel;
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

    tree->bridges_buffer = gpu_create_buffer(tree->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, tree->base.max_cells * sizeof(CellBridge));
    tree->cells_buffer = gpu_create_buffer(tree->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, tree->base.max_cells * sizeof(GPUCell));
    tree->next_indices_buffer = gpu_create_buffer(tree->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, TAY_MAX_AGENTS * sizeof(int));

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            tree->agent_buffers[i] = gpu_create_buffer(tree->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, group->capacity * group->agent_size);
    }

    /* compose kernels source text */

    tree->text_size = 0;
    tree->text_size += sprintf_s(tree->text + tree->text_size, GPU_TREE_MAX_TEXT_SIZE - tree->text_size, HEADER, TAY_MAX_GROUPS, TAY_MAX_DIMENSIONS, GPU_TREE_NULL_INDEX);

    /* build program */

    gpu_build_program(tree->gpu, tree->text);

    /* create kernels */

    tree->resolve_cell_pointers_kernel = gpu_create_kernel(tree->gpu, "resolve_cell_pointers");
    gpu_set_kernel_buffer_argument(tree->resolve_cell_pointers_kernel, 0, &tree->bridges_buffer);
    gpu_set_kernel_buffer_argument(tree->resolve_cell_pointers_kernel, 1, &tree->cells_buffer);

    tree->resolve_cell_agent_pointers_kernel = gpu_create_kernel(tree->gpu, "resolve_cell_agent_pointers");
    gpu_set_kernel_buffer_argument(tree->resolve_cell_agent_pointers_kernel, 0, &tree->bridges_buffer);
    gpu_set_kernel_buffer_argument(tree->resolve_cell_agent_pointers_kernel, 1, &tree->cells_buffer);
    gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 2, &tree->base.cells_count, sizeof(tree->base.cells_count));

    tree->resolve_agent_pointers_kernel = gpu_create_kernel(tree->gpu, "resolve_agent_pointers");
    gpu_set_kernel_buffer_argument(tree->resolve_agent_pointers_kernel, 1, &tree->next_indices_buffer);

    /* push agents to GPU */

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            gpu_enqueue_write_buffer(tree->gpu, tree->agent_buffers[i], GPU_BLOCKING, group->capacity * group->agent_size, group->storage);
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

    CellBridge *bridges = (CellBridge *)tree->arena;
    assert(tree->base.cells_count * sizeof(CellBridge) < GPU_TREE_ARENA_SIZE);

    /* translate cells into cell bridges */
    for (int i = 0; i < base->cells_count; ++i) {
        Cell *cell = base->cells + i;
        CellBridge *bridge = bridges + i;

        bridge->box = cell->box;

        bridge->lo = (cell->lo != 0) ? (int)(cell->lo - base->cells) : GPU_TREE_NULL_INDEX;
        bridge->hi = (cell->hi != 0) ? (int)(cell->hi - base->cells) : GPU_TREE_NULL_INDEX;

        for (int j = 0; j < TAY_MAX_GROUPS; ++j)
            bridge->first[j] = group_tag_to_index(state->groups + j, cell->first[j]);
    }

    /* send bridges to GPU */

    gpu_enqueue_write_buffer(tree->gpu, tree->bridges_buffer, GPU_BLOCKING, tree->base.cells_count * sizeof(CellBridge), bridges);

    /* resolve pointers on GPU side */

    gpu_enqueue_kernel(tree->gpu, tree->resolve_cell_pointers_kernel, base->cells_count);

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {

            /* resolve cell agent pointers */

            gpu_set_kernel_buffer_argument(tree->resolve_cell_agent_pointers_kernel, 3, &tree->agent_buffers[i]);
            gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 4, &group->agent_size, sizeof(group->agent_size));
            gpu_set_kernel_value_argument(tree->resolve_cell_agent_pointers_kernel, 5, &i, sizeof(i));

            gpu_enqueue_kernel(tree->gpu, tree->resolve_cell_agent_pointers_kernel, base->cells_count);

            /* resolve agent next pointers */

            int *next_indices = (int *)tree->arena;
            assert(group->capacity * sizeof(int) < GPU_TREE_ARENA_SIZE);

            for (TayAgentTag *tag = group->first; tag; tag = tag->next) {
                int this_i = group_tag_to_index(group, tag);
                int next_i = group_tag_to_index(group, tag->next);
                next_indices[this_i] = next_i;
            }

            gpu_enqueue_write_buffer(tree->gpu, tree->next_indices_buffer, GPU_BLOCKING, group->capacity * sizeof(int), next_indices);

            gpu_set_kernel_buffer_argument(tree->resolve_agent_pointers_kernel, 0, &tree->agent_buffers[i]);
            gpu_set_kernel_value_argument(tree->resolve_agent_pointers_kernel, 2, &group->agent_size, sizeof(group->agent_size));

            gpu_enqueue_kernel(tree->gpu, tree->resolve_agent_pointers_kernel, group->capacity);
        }
    }
}

static void _see(TayState *state, int pass_index) {
}

static void _act(TayState *state, int pass_index) {
}

void space_gpu_tree_init(TaySpaceContainer *container, int dims, float *radii, int max_depth_correction) {
    space_container_init(container, _init(dims, radii, max_depth_correction), dims, _destroy);
    container->on_simulation_start = _on_simulation_start;
    container->on_simulation_end = _on_simulation_end;
    container->on_step_start = _on_step_start;
    container->add = _add;
    container->see = _see;
    container->act = _act;
}
