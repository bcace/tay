#include "state.h"
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MAX_KERNEL_SOURCE_LENGTH 10000


static const char *HEADER = "\n\
#define DIMS %d\n\
\n\
#define AGENT_POSITION_PTR(__agent_tag__) ((global float4 *)(__agent_tag__ + 1))\n\
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
}\n";

static const char *SEE_KERNEL = "\n\
kernel void %s_kernel(global char *seer_agents, int seer_agent_size, global char *seen_agents, int seen_agent_size, int first_seen, float4 radii, global void *see_context) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *seer_agent = (global TayAgentTag *)(seer_agents + i * seer_agent_size);\n\
    float4 seer_p = *AGENT_POSITION_PTR(seer_agent);\n\
    global TayAgentTag *seen_agent = (global TayAgentTag *)(seen_agents + first_seen * seen_agent_size);\n\
\n\
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
    GpuKernel pass_kernels[TAY_MAX_PASSES];
    GpuBuffer next_indices_buffer;
    char kernels_source[MAX_KERNEL_SOURCE_LENGTH];
    int bytes_written;
    int agent_capacity;
    int *next_indices;
} Space;

static Space *_init() {
    Space *s = malloc(sizeof(Space));
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        s->first[i] = 0;
    s->gpu = gpu_create();
    s->kernels_source[0] = '\0';
    s->bytes_written = 0;
    s->agent_capacity = 100000;
    s->next_indices = malloc(sizeof(int) * s->agent_capacity);
    return s;
}

static void _destroy(TaySpaceContainer *container) {
    Space *s = (Space *)container->storage;
    gpu_destroy(s->gpu);
    free(s->next_indices);
    free(s);
}

static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
    Space *s = (Space *)container->storage;
    agent->next = s->first[group];
    s->first[group] = agent;
}

static void _on_simulation_start(TayState *state) {
    Space *s = (Space *)state->space.storage;

    /* create next indices buffer */
    s->next_indices_buffer = gpu_create_buffer(s->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, s->agent_capacity * sizeof(int));

    /* create agent buffers */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            s->agent_buffers[i] = gpu_create_buffer(s->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, group->capacity * group->agent_size);
    }

    s->bytes_written += sprintf_s(s->kernels_source, MAX_KERNEL_SOURCE_LENGTH, HEADER,
                                  state->space.dims);

    /* copy agent structs and behaviors code into the buffer */
    s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, state->source);

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;

        /* create buffer for pass context */
        if (pass->context)
            s->pass_context_buffers[i] = gpu_create_buffer(s->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, pass->context_size);
        else
            s->pass_context_buffers[i] = 0;

        /* append kernel function wrapper around the pass agent function */
        if (pass->type == TAY_PASS_SEE)
            s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, SEE_KERNEL,
                                          pass->func_name, pass->func_name);
        else if (pass->type == TAY_PASS_ACT)
            s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, ACT_KERNEL,
                                          pass->func_name, pass->func_name);
        else
            assert(0); /* unhandled pass type */

        assert(s->bytes_written < MAX_KERNEL_SOURCE_LENGTH);
    }

    /* build program */

    assert(state->source != 0);
    gpu_build_program(s->gpu, s->kernels_source);

    /* create kernels and bind buffers to them */

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;

        static char pass_kernel_name[512];
        sprintf_s(pass_kernel_name, 512, "%s_kernel", pass->func_name);

        GpuKernel kernel = 0;

        if (pass->type == TAY_PASS_SEE) {
            kernel = gpu_create_kernel(s->gpu, pass_kernel_name);
            TayGroup *seer_group = state->groups + pass->seer_group;
            TayGroup *seen_group = state->groups + pass->seen_group;
            int first_seen = group_tag_to_index(seen_group, s->first[pass->seen_group]);
            gpu_set_kernel_buffer_argument(kernel, 0, &s->agent_buffers[pass->seer_group]);
            gpu_set_kernel_value_argument(kernel, 1, &seer_group->agent_size, sizeof(seer_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &s->agent_buffers[pass->seen_group]);
            gpu_set_kernel_value_argument(kernel, 3, &seen_group->agent_size, sizeof(seen_group->agent_size));
            gpu_set_kernel_value_argument(kernel, 4, &first_seen, sizeof(first_seen));
            gpu_set_kernel_value_argument(kernel, 5, &pass->radii, sizeof(pass->radii));
            gpu_set_kernel_buffer_argument(kernel, 6, &s->pass_context_buffers[i]);
        }
        else if (pass->type == TAY_PASS_ACT) {
            kernel = gpu_create_kernel(s->gpu, pass_kernel_name);
            TayGroup *act_group = state->groups + pass->act_group;
            gpu_set_kernel_buffer_argument(kernel, 0, &s->agent_buffers[pass->act_group]);
            gpu_set_kernel_value_argument(kernel, 1, &act_group->agent_size, sizeof(act_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &s->pass_context_buffers[i]);
        }
        else
            assert(0); /* unhandled pass type */

        s->pass_kernels[i] = kernel;
    }

    GpuKernel resolve_pointers_kernel = gpu_create_kernel(s->gpu, "resolve_pointers");

    /* enqueue writing agents to GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {
            gpu_enqueue_write_buffer(s->gpu, s->agent_buffers[i], GPU_BLOCKING, group->capacity * group->agent_size, group->storage);

            for (TayAgentTag *tag = s->first[i]; tag; tag = tag->next) {
                int this_i = group_tag_to_index(group, tag);
                int next_i = group_tag_to_index(group, tag->next);
                s->next_indices[this_i] = next_i;
            }

            gpu_enqueue_write_buffer(s->gpu, s->next_indices_buffer, GPU_BLOCKING, group->capacity * sizeof(int), s->next_indices);

            gpu_set_kernel_buffer_argument(resolve_pointers_kernel, 0, &s->agent_buffers[i]);
            gpu_set_kernel_buffer_argument(resolve_pointers_kernel, 1, &s->next_indices_buffer);
            gpu_set_kernel_value_argument(resolve_pointers_kernel, 2, &group->agent_size, sizeof(group->agent_size));

            gpu_enqueue_kernel(s->gpu, resolve_pointers_kernel, group->capacity);
        }
    }

    gpu_release_kernel(resolve_pointers_kernel);

    /* enqueue writing pass contexts to GPU */
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        gpu_enqueue_write_buffer(s->gpu, s->pass_context_buffers[i], GPU_BLOCKING, pass->context_size, pass->context);
    }
}

static void _see(TayState *state, int pass_index) {
    Space *s = state->space.storage;
    TayPass *pass = state->passes + pass_index;
    TayGroup *seer_group = state->groups + pass->seer_group;
    gpu_enqueue_kernel(s->gpu, s->pass_kernels[pass_index], seer_group->capacity);
}

static void _act(TayState *state, int pass_index) {
    Space *s = state->space.storage;
    TayPass *pass = state->passes + pass_index;
    TayGroup *act_group = state->groups + pass->act_group;
    gpu_enqueue_kernel(s->gpu, s->pass_kernels[pass_index], act_group->capacity);
}

static void _on_run_end(TaySpaceContainer *container, TayState *state) {
    Space *s = (Space *)container->storage;

    /* enqueue reading agents from GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            gpu_enqueue_read_buffer(s->gpu, s->agent_buffers[i], GPU_BLOCKING, group->capacity * group->agent_size, group->storage);
    }
}

void space_gpu_simple_init(TaySpaceContainer *container, int dims) {
    space_container_init(container, _init(), dims, _destroy);
    container->add = _add;
    container->see = _see;
    container->act = _act;
    container->on_simulation_start = _on_simulation_start;
    container->on_run_end = _on_run_end;
}
