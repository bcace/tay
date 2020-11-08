#include "state.h"
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


static const char *HEADER = "\n\
#define DIMS %d\n\
#define TAY_GPU_DEAD 0x%llx\n\
\n\
#define TAY_AGENT_POSITION(__agent_tag__) (*(global float4 *)(__agent_tag__ + 1))\n\
\n\
\n\
typedef struct __attribute__((packed)) TayAgentTag {\n\
    global struct TayAgentTag *next;\n\
} TayAgentTag;\n\
\n\
kernel void resolve_pointers(global char *agents, global int *next_indices, int agent_size) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *tag = (global TayAgentTag *)(agents + i * agent_size);\n\
    if (next_indices[i] != -1)\n\
        tag->next = (global TayAgentTag *)(agents + next_indices[i] * agent_size);\n\
    else\n\
        tag->next = 0;\n\
}\n\
kernel void mark_dead_agents(global char *agents, int agent_size, global int *dead_agent_indices) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *tag = (global TayAgentTag *)(agents + i * agent_size);\n\
    tag->next = TAY_GPU_DEAD;\n\
}\n";

static const char *SEE_KERNEL = "\n\
kernel void %s_kernel(global char *seer_agents, int seer_agent_size, global char *seen_agents, int seen_agent_size, int first_seen, float4 radii, global void *see_context) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *seer_agent = (global TayAgentTag *)(seer_agents + i * seer_agent_size);\n\
    float4 seer_p = TAY_AGENT_POSITION(seer_agent);\n\
    global TayAgentTag *seen_agent = (global TayAgentTag *)(seen_agents + first_seen * seen_agent_size);\n\
\n\
    while (seen_agent) {\n\
        float4 seen_p = TAY_AGENT_POSITION(seen_agent);\n\
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
        %s(seer_agent, seen_agent, see_context);\n\
\n\
        SKIP_SEE:;\n\
\n\
        seen_agent = seen_agent->next;\n\
    }\n\
}\n";

static const char *ACT_KERNEL = "\n\
kernel void %s_kernel(global char *agents, int agent_size, global void *act_context) {\n\
    %s((global void *)(agents + get_global_id(0) * agent_size), act_context);\n\
}\n";

typedef struct {
    TayAgentTag *first[TAY_MAX_GROUPS];
    GpuContext *gpu;
    GpuBuffer agent_buffers[TAY_MAX_GROUPS];
    GpuBuffer pass_context_buffers[TAY_MAX_PASSES];
    GpuBuffer agent_io_buffer;
    GpuKernel pass_kernels[TAY_MAX_PASSES];
    GpuKernel resolve_pointers_kernel;
    char text[TAY_GPU_MAX_TEXT_SIZE];
    int text_size;
    char arena[TAY_GPU_ARENA_SIZE];
} Simple;

static Simple *_init() {
    Simple *simple = malloc(sizeof(Simple));
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        simple->first[i] = 0;
    simple->gpu = gpu_create();
    simple->text_size = 0;
    return simple;
}

static void _destroy(TaySpaceContainer *container) {
    Simple *simple = (Simple *)container->storage;
    gpu_destroy(simple->gpu);
    free(simple);
}

static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
    Simple *simple = (Simple *)container->storage;
    agent->next = simple->first[group];
    simple->first[group] = agent;
}

static void _on_simulation_start(TayState *state) {
    Simple *simple = state->space.storage;

    /* create buffers */

    simple->agent_io_buffer = gpu_create_buffer(simple->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, TAY_MAX_AGENTS * sizeof(int));

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            simple->agent_buffers[i] = gpu_create_buffer(simple->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, group->capacity * group->agent_size);
    }

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        simple->pass_context_buffers[i] = gpu_create_buffer(simple->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, pass->context_size);
    }

    /* compose kernels source text */

    simple->text_size = 0;
    simple->text_size += sprintf_s(simple->text + simple->text_size, TAY_GPU_MAX_TEXT_SIZE - simple->text_size, HEADER,
                                   state->space.dims, TAY_GPU_DEAD);

    simple->text_size += sprintf_s(simple->text + simple->text_size, TAY_GPU_MAX_TEXT_SIZE - simple->text_size, state->source);

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            simple->text_size += sprintf_s(simple->text + simple->text_size, TAY_GPU_MAX_TEXT_SIZE - simple->text_size, SEE_KERNEL,
                                           pass->func_name, pass->func_name);
        else if (pass->type == TAY_PASS_ACT)
            simple->text_size += sprintf_s(simple->text + simple->text_size, TAY_GPU_MAX_TEXT_SIZE - simple->text_size, ACT_KERNEL,
                                           pass->func_name, pass->func_name);
        else
            assert(0); /* unhandled pass type */
    }

    assert(simple->text_size < TAY_GPU_MAX_TEXT_SIZE);

    /* build program */

    gpu_build_program(simple->gpu, simple->text);

    /* create kernels */

    simple->resolve_pointers_kernel = gpu_create_kernel(simple->gpu, "resolve_pointers");

    /* pass kernels */

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;

        static char pass_kernel_name[512];
        sprintf_s(pass_kernel_name, 512, "%s_kernel", pass->func_name);

        GpuKernel kernel = 0;

        if (pass->type == TAY_PASS_SEE) {
            kernel = gpu_create_kernel(simple->gpu, pass_kernel_name);
            TayGroup *seer_group = state->groups + pass->seer_group;
            TayGroup *seen_group = state->groups + pass->seen_group;
            int first_seen = group_tag_to_index(seen_group, simple->first[pass->seen_group]);
            gpu_set_kernel_buffer_argument(kernel, 0, &simple->agent_buffers[pass->seer_group]);
            gpu_set_kernel_value_argument(kernel, 1, &seer_group->agent_size, sizeof(seer_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &simple->agent_buffers[pass->seen_group]);
            gpu_set_kernel_value_argument(kernel, 3, &seen_group->agent_size, sizeof(seen_group->agent_size));
            gpu_set_kernel_value_argument(kernel, 4, &first_seen, sizeof(first_seen));
            gpu_set_kernel_value_argument(kernel, 5, &pass->radii, sizeof(pass->radii));
            gpu_set_kernel_buffer_argument(kernel, 6, &simple->pass_context_buffers[i]);
        }
        else if (pass->type == TAY_PASS_ACT) {
            kernel = gpu_create_kernel(simple->gpu, pass_kernel_name);
            TayGroup *act_group = state->groups + pass->act_group;
            gpu_set_kernel_buffer_argument(kernel, 0, &simple->agent_buffers[pass->act_group]);
            gpu_set_kernel_value_argument(kernel, 1, &act_group->agent_size, sizeof(act_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &simple->pass_context_buffers[i]);
        }
        else
            assert(0); /* unhandled pass type */

        simple->pass_kernels[i] = kernel;
    }

    /* enqueue writing agents to GPU */

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {
            gpu_enqueue_write_buffer(simple->gpu, simple->agent_buffers[i], GPU_BLOCKING, group->capacity * group->agent_size, group->storage);

            assert(group->capacity * sizeof(int) < TAY_GPU_ARENA_SIZE);
            int *next_indices = (int *)simple->arena;

            for (TayAgentTag *tag = simple->first[i]; tag; tag = tag->next) {
                int this_i = group_tag_to_index(group, tag);
                int next_i = group_tag_to_index(group, tag->next);
                next_indices[this_i] = next_i;
            }

            gpu_enqueue_write_buffer(simple->gpu, simple->agent_io_buffer, GPU_BLOCKING, group->capacity * sizeof(int), next_indices);

            gpu_set_kernel_buffer_argument(simple->resolve_pointers_kernel, 0, &simple->agent_buffers[i]);
            gpu_set_kernel_buffer_argument(simple->resolve_pointers_kernel, 1, &simple->agent_io_buffer);
            gpu_set_kernel_value_argument(simple->resolve_pointers_kernel, 2, &group->agent_size, sizeof(group->agent_size));

            gpu_enqueue_kernel(simple->gpu, simple->resolve_pointers_kernel, group->capacity);
        }
    }

    /* enqueue writing pass contexts to GPU */

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        gpu_enqueue_write_buffer(simple->gpu, simple->pass_context_buffers[i], GPU_BLOCKING, pass->context_size, pass->context);
    }
}

static void _see(TayState *state, int pass_index) {
    Simple *simple = state->space.storage;
    TayPass *pass = state->passes + pass_index;
    TayGroup *seer_group = state->groups + pass->seer_group;
    gpu_enqueue_kernel(simple->gpu, simple->pass_kernels[pass_index], seer_group->capacity);
}

static void _act(TayState *state, int pass_index) {
    Simple *simple = state->space.storage;
    TayPass *pass = state->passes + pass_index;
    TayGroup *act_group = state->groups + pass->act_group;
    gpu_enqueue_kernel(simple->gpu, simple->pass_kernels[pass_index], act_group->capacity);
}

static void _on_run_end(TaySpaceContainer *container, TayState *state) {
    Simple *simple = (Simple *)container->storage;

    /* enqueue reading agents from GPU */

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            gpu_enqueue_read_buffer(simple->gpu, simple->agent_buffers[i], GPU_NON_BLOCKING, group->capacity * group->agent_size, group->storage);
    }

    gpu_finish(simple->gpu);

    /* reset all agents "next" indices since they currently contain GPU addresses */

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {

            group->first = 0;
            simple->first[i] = 0;

            for (int j = 0; j < group->capacity; ++j) {
                TayAgentTag *tag = (TayAgentTag *)((char *)group->storage + j * group->agent_size);

                if ((unsigned long long)tag == TAY_GPU_DEAD) { /* dead agents, return to storage */
                    tag->next = group->first;
                    group->first = tag;
                }
                else { /* live agents, return to tree list */
                    tag->next = simple->first[i];
                    simple->first[i] = tag;
                }
            }
        }
    }
}

static void _on_simulation_end(TayState *state) {
    Simple *simple = state->space.storage;
    gpu_release_kernel(simple->resolve_pointers_kernel);
    for (int i = 0; i < state->passes_count; ++i)
        gpu_release_kernel(simple->pass_kernels[i]);
}

void space_gpu_simple_init(TaySpaceContainer *container, int dims) {
    space_container_init(container, _init(), dims, _destroy);
    container->add = _add;
    container->see = _see;
    container->act = _act;
    container->on_simulation_start = _on_simulation_start;
    container->on_run_end = _on_run_end;
    container->on_simulation_end = _on_simulation_end;
}
