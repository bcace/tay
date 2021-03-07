#include "space.h"
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#if TAY_GPU


void gpu_shared_create_global(TayState *state) {
    GpuShared *shared = &state->gpu_shared;
    if (shared->gpu == 0)
        shared->gpu = gpu_create();
}

/* Sends indices representing agent "next" pointers to GPU. Assumes the
"next_indices" was already filled for live agents (only traversable by
the specific space), and only specially marks dead agents. */
void gpu_shared_finish_fixing_group_gpu_pointers(GpuShared *shared, Space *space, TayGroup *group, int group_i, int *next_indices) {

    /* set all dead agents' next indices to a special value */
    for (TayAgentTag *tag = group->first; tag; tag = tag->next) {
        int this_i = group_tag_to_index(group, tag);
        next_indices[this_i] = TAY_GPU_DEAD_INDEX;
    }

    gpu_enqueue_write_buffer(shared->gpu, space->gpu_shared_buffer, GPU_BLOCKING, group->capacity * sizeof(int), next_indices);
    gpu_set_kernel_buffer_argument(shared->resolve_pointers_kernel, 0, &shared->agent_buffers[group_i]);
    gpu_set_kernel_buffer_argument(shared->resolve_pointers_kernel, 1, &space->gpu_shared_buffer);
    gpu_set_kernel_value_argument(shared->resolve_pointers_kernel, 2, &group->agent_size, sizeof(group->agent_size));
    gpu_enqueue_kernel(shared->gpu, shared->resolve_pointers_kernel, group->capacity);
}

// TODO: is it OK that DIMS is here? It's supposed to be space-specific
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
}\n";

static void _create_model_related_buffers_and_kernels(TayState *state, const char *source) {
    GpuShared *shared = &state->gpu_shared;

    /* compose program source text */
    {
        shared->text_size = 0;
        shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, HEADER,
                                       3, TAY_GPU_DEAD_ADDR, TAY_GPU_NULL_INDEX, TAY_GPU_DEAD_INDEX);
        shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, source);

        gpu_simple_contribute_source(state);
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
        shared->resolve_pointers_kernel = gpu_create_kernel(shared->gpu, "fix_pointers");
    }

    /* create private kernels */
    for (int i = 0; i < state->spaces_count; ++i) {
        Space *space = state->spaces + i;

        if ((space->type & ST_GPU) == 0)
            continue;

        switch (space->type) {
            case ST_GPU_SIMPLE_DIRECT: gpu_simple_create_kernels(state); break;
            case ST_GPU_SIMPLE_INDIRECT: gpu_simple_create_kernels(state); break;
            default: assert(false); /* not implemented */
        }

        space->gpu_shared_buffer = gpu_create_buffer(shared->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, space->shared_size);
    }
}

static void _destroy_model_related_buffers_and_kernels(TayState *state) {
    GpuShared *shared = &state->gpu_shared;

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
        gpu_release_kernel(shared->resolve_pointers_kernel);
    }

    /* release space-specific kernels */
    for (int i = 0; i < state->spaces_count; ++i) {
        Space *space = state->spaces + i;

        if ((space->type & ST_GPU) == 0)
            continue;

        switch (space->type) {
            case ST_GPU_SIMPLE_DIRECT: gpu_simple_destroy_kernels(state); break;
            case ST_GPU_SIMPLE_INDIRECT: gpu_simple_destroy_kernels(state); break;
            default: assert(false); /* not implemented */
        }

        gpu_release_buffer(space->gpu_shared_buffer);
    }
}

void gpu_shared_refresh_model_related_kernels_and_buffers(TayState *state, const char *source) {
    _destroy_model_related_buffers_and_kernels(state);
    _create_model_related_buffers_and_kernels(state, source);
}

void gpu_shared_destroy_global(TayState *state) {
    GpuShared *global = &state->gpu_shared;
    if (global->gpu) {
        _destroy_model_related_buffers_and_kernels(state);
        gpu_destroy(global->gpu);
        global->gpu = 0;
    }
}

/* Copies all agents to GPU. Agent "next" pointers should be updated at the
beginning of each step. */
void gpu_shared_push_agents_and_pass_contexts(TayState *state) {
    GpuShared *shared = &state->gpu_shared;

    /* push agents to GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if ((group->space->type & ST_GPU) && group->storage)
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
void gpu_shared_fetch_agents(TayState *state) {
    GpuShared *shared = &state->gpu_shared;

    /* fetch agents from GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage && (group->space->type & ST_GPU))
            gpu_enqueue_read_buffer(shared->gpu, shared->agent_buffers[i], GPU_NON_BLOCKING, group->capacity * group->agent_size, group->storage);
    }

    gpu_finish(shared->gpu);

    /* reset all gpu spaces' bounding boxes */
    for (int i = 0; i < state->spaces_count; ++i) {
        Space *space = state->spaces + i;
        if (space->type & ST_GPU)
            box_reset(&space->box, space->dims);
    }

    /* fix next pointers since we just got the ones from GPU, and update the space bounding box */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;

        if (group->storage && (group->space->type & ST_GPU)) { /* valid gpu group */
            Space *space = group->space;

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

#endif /* TAY_GPU */
