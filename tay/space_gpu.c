#include "space.h"
#include "gpu.h"
#include <stdio.h>
#include <assert.h>


static const char *HEADER = "\n\
#define DIMS %d\n\
#define TAY_GPU_DEAD_ADDR 0x%llx\n\
#define TAY_GPU_NULL_INDEX %d\n\
#define TAY_GPU_DEAD_INDEX %d\n\
\n\
#define float4_agent_position(__agent_tag__) (*(global float4 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
#define float3_agent_position(__agent_tag__) (*(global float3 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
#define float2_agent_position(__agent_tag__) (*(global float2 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
\n\
\n\
typedef struct __attribute__((packed)) TayAgentTag {\n\
    global struct TayAgentTag *next;\n\
} TayAgentTag;\n\
\n\
kernel void fix_pointers(global char *agents, global int *next_indices, int agent_size) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *tag = (global TayAgentTag *)(agents + i * agent_size);\n\
    if (next_indices[i] == TAY_GPU_NULL_INDEX)\n\
        tag->next = 0;\n\
    else if (next_indices[i] == TAY_GPU_DEAD_INDEX)\n\
        tag->next = (global TayAgentTag *)TAY_GPU_DEAD_ADDR;\n\
    else\n\
        tag->next = (global TayAgentTag *)(agents + next_indices[i] * agent_size);\n\
}\n\
\n\
kernel void fetch_new_positions(global char *agents, global float4 *positions, int agent_size) {\n\
    int i = get_global_id(0);\n\
    positions[i] = *(global float4 *)(agents + i * agent_size + sizeof(TayAgentTag));\n\
}\n";

/* Called when the state is initialized. */
void space_gpu_shared_init(GpuShared *shared) {
    shared->gpu = gpu_create();
}

/* Called when the state is destroyed. */
void space_gpu_shared_release(GpuShared *shared) {
    gpu_destroy(shared->gpu);
}

void space_gpu_on_simulation_start(TayState *state) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;

    /* compose program source text */
    {
        shared->text_size = 0;
        shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, HEADER,
                                       space->dims, TAY_GPU_DEAD_ADDR, TAY_GPU_NULL_INDEX, TAY_GPU_DEAD_INDEX);
        shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, state->source);
        gpu_simple_add_source(state);
        gpu_tree_add_source(state);
    }

    /* build program */
    {
        assert(shared->text_size < TAY_GPU_MAX_TEXT_SIZE);
        gpu_build_program(shared->gpu, shared->text);
    }

    /* create agent buffers on GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            shared->agent_buffers[i] = gpu_create_buffer(shared->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, group->capacity * group->agent_size);
    }

    /* create pass context buffers on GPU */
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        shared->pass_context_buffers[i] = gpu_create_buffer(shared->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, pass->context_size);
    }

    /* create other shared buffers and kernels */
    {
        shared->agent_io_buffer = gpu_create_buffer(shared->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, TAY_CPU_SHARED_TEMP_ARENA_SIZE);
        shared->cells_buffer = gpu_create_buffer(shared->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, TAY_CPU_SHARED_CELL_ARENA_SIZE);

        shared->resolve_pointers_kernel = gpu_create_kernel(shared->gpu, "fix_pointers");
        gpu_set_kernel_buffer_argument(shared->resolve_pointers_kernel, 1, &shared->agent_io_buffer);

        shared->fetch_new_positions_kernel = gpu_create_kernel(shared->gpu, "fetch_new_positions");
        gpu_set_kernel_buffer_argument(shared->fetch_new_positions_kernel, 1, &shared->agent_io_buffer);
    }

    /* create private buffers and kernels */
    {
        gpu_simple_on_simulation_start(state);
        gpu_tree_on_simulation_start(state);
    }
}

void space_gpu_on_simulation_end(TayState *state) {
    GpuShared *shared = &state->space.gpu_shared;

    /* release agent buffers on GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            gpu_release_buffer(shared->agent_buffers[i]);
    }

    /* release pass kernels and context buffers */
    for (int i = 0; i < state->passes_count; ++i)
        gpu_release_buffer(shared->pass_context_buffers[i]);

    /* release other shared buffers and kernels */
    {
        gpu_release_buffer(shared->agent_io_buffer);
        gpu_release_buffer(shared->cells_buffer);
        gpu_release_kernel(shared->resolve_pointers_kernel);
        gpu_release_kernel(shared->fetch_new_positions_kernel);
    }

    /* release private buffers and kernels */
    {
        gpu_simple_on_simulation_end(state);
        gpu_tree_on_simulation_end(state);
    }
}

/* Copies all agents to GPU. Agent "next" pointers should be updated at the
beginning of each step. */
void space_gpu_push_agents(TayState *state) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;

    /* push agents to GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            gpu_enqueue_write_buffer(shared->gpu, shared->agent_buffers[i], GPU_NON_BLOCKING, group->capacity * group->agent_size, group->storage);
    }

    /* push pass contexts to GPU */
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        gpu_enqueue_write_buffer(shared->gpu, shared->pass_context_buffers[i], GPU_NON_BLOCKING, pass->context_size, pass->context);
    }

    gpu_finish(shared->gpu);
}

/* Copies all agents from GPU, returns them to space and group storage,
and updates the space box. */
void space_gpu_fetch_agents(TayState *state) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;

    box_reset(&space->box, space->dims);

    /* fetch agents from GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            gpu_enqueue_read_buffer(shared->gpu, shared->agent_buffers[i], GPU_NON_BLOCKING, group->capacity * group->agent_size, group->storage);
    }

    gpu_finish(shared->gpu);

    /* fix next pointers since we just got the ones from GPU, and update the space bounding box */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {

            group->first = 0;
            space->first[i] = 0;
            space->counts[i] = 0;

            for (int j = 0; j < group->capacity; ++j) {
                TayAgentTag *tag = (TayAgentTag *)((char *)group->storage + j * group->agent_size);

                if ((unsigned long long)tag->next == TAY_GPU_DEAD_ADDR) { /* dead agents, return to storage */
                    tag->next = group->first;
                    group->first = tag;
                }
                else { /* live agents, return to space */
                    tag->next = space->first[i];
                    space->first[i] = tag;
                    ++space->counts[i];
                    box_update(&space->box, float4_agent_position(tag), space->dims);
                }
            }
        }
    }
}

/* Sends indices representing agent "next" pointers to GPU. Assumes the
"next_indices" was already filled for live agents (only traversable by
the specific space), and only only specially marks dead agents. */
void space_gpu_finish_fixing_group_gpu_pointers(GpuShared *shared, TayGroup *group, int group_i, int *next_indices) {

    /* set all dead agents' next indices to a special value */
    for (TayAgentTag *tag = group->first; tag; tag = tag->next) {
        int this_i = group_tag_to_index(group, tag);
        next_indices[this_i] = TAY_GPU_DEAD_INDEX;
    }

    gpu_enqueue_write_buffer(shared->gpu, shared->agent_io_buffer, GPU_BLOCKING, group->capacity * sizeof(int), next_indices);
    gpu_set_kernel_buffer_argument(shared->resolve_pointers_kernel, 0, &shared->agent_buffers[group_i]);
    gpu_set_kernel_value_argument(shared->resolve_pointers_kernel, 2, &group->agent_size, sizeof(group->agent_size));
    gpu_enqueue_kernel(shared->gpu, shared->resolve_pointers_kernel, group->capacity);
}

/* Gets new agent positions from GPU. Used when we only need new agent positions
on CPU for the next step to be able to update space partitioning. */
void space_gpu_fetch_agent_positions(TayState *state) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;

    box_reset(&space->box, space->dims);

    float4 *positions = space_get_temp_arena(space, TAY_MAX_AGENTS * sizeof(float4) * 2);

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {

            int req_size = group->capacity * sizeof(float4) * (group->is_point ? 1 : 2);

            gpu_set_kernel_buffer_argument(shared->fetch_new_positions_kernel, 0, &shared->agent_buffers[i]);
            gpu_set_kernel_value_argument(shared->fetch_new_positions_kernel, 2, &group->agent_size, sizeof(group->agent_size));
            gpu_enqueue_kernel(shared->gpu, shared->fetch_new_positions_kernel, group->capacity);
            gpu_enqueue_read_buffer(shared->gpu, shared->agent_io_buffer, GPU_BLOCKING, req_size, positions);

            /* copy new positions into agents */
            for (int i = 0; i < group->capacity; ++i) {
                float4_agent_position((char *)group->storage + group->agent_size * i) = positions[i];
                box_update(&space->box, positions[i], space->dims);
            }
        }
    }
}
