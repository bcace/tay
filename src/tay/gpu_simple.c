#include "state.h"
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MAX_KERNEL_SOURCE_LENGTH 10000


const char *RESOLVE_INDICES_KERNEL = "kernel void resolve_pointers(global char *agents, global int *next_indices, int agent_size) {\n\
    int id = get_global_id(0);\n\
    int next_index = next_indices[id];\n\
    global TayAgentTag *tag = (global TayAgentTag *)(agents + id * agent_size);\n\
    if (next_index != -1)\n\
        tag->next = (global TayAgentTag *)(agents + next_index * agent_size);\n\
    else\n\
        tag->next = 0;\n\
}\n";

/* TODO: both these kernels should test whether the agent is alive at all.
This could be done by looking the agent up in the info array. */

// TODO: revise pointer arithmetic once the kernel is made to be more generic
const char *SEE_KERNEL_SOURCE = "kernel void pass_%d(global Agent *seer_agents, global Agent *seen_agents, global SeeContext *see_context, int first_seen, int dims) {\n\
    global Agent *seer_agent = seer_agents + get_global_id(0);\n\
    global TayAgentTag *tag = (global TayAgentTag *)(seen_agents + first_seen);\n\
    while (tag) {\n\
        global Agent *seen_agent = (global Agent *)tag;\n\
        \n\
        if (seer_agent == seen_agent)\n\
            goto SKIP_SEE;\n\
        \n\
        float4 d = seer_agent->p - seen_agent->p;\n\
        if (d.x > see_context->radii.x || d.x < -see_context->radii.x)\n\
            goto SKIP_SEE;\n\
        if (dims > 1) {\n\
            if (d.y > see_context->radii.y || d.y < -see_context->radii.y)\n\
                goto SKIP_SEE;\n\
        }\n\
        if (dims > 2) {\n\
            if (d.z > see_context->radii.z || d.z < -see_context->radii.z)\n\
                goto SKIP_SEE;\n\
        }\n\
        if (dims > 3) {\n\
            if (d.w > see_context->radii.w || d.w < -see_context->radii.w)\n\
                goto SKIP_SEE;\n\
        }\n\
        \n\
        %s(seer_agent, seen_agent, see_context);\n\
        \n\
        SKIP_SEE:;\n\
        \n\
        tag = tag->next;\n\
    }\n\
}\n";

const char *ACT_KERNEL_SOURCE = "kernel void pass_%d(global Agent *agents, global ActContext *act_context) {\n\
    %s(agents + get_global_id(0), act_context);\n\
}\n";

typedef struct {
    TayAgentTag *first[TAY_MAX_GROUPS];
    GpuContext *gpu;
    GpuBuffer agent_buffers[TAY_MAX_GROUPS];
    GpuBuffer pass_contexts[TAY_MAX_PASSES];
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

static inline int _agent_index(TayGroup *group, TayAgentTag *tag) {
    return (tag != 0) ? (int)((char *)tag - (char *)group->storage) / group->agent_size : -1;
}

static void _on_simulation_start(TaySpaceContainer *container, TayState *state) {
    Space *s = (Space *)container->storage;

    /* create next indices buffer */
    s->next_indices_buffer = gpu_create_buffer(s->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, s->agent_capacity * sizeof(int));

    /* create agent buffers */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            s->agent_buffers[i] = gpu_create_buffer(s->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, group->capacity * group->agent_size);
    }

    /* copy agent structs and behaviors code into the buffer */
    s->bytes_written += sprintf_s(s->kernels_source, MAX_KERNEL_SOURCE_LENGTH, state->source);

    /* append pointer resolve kernel into the buffer */
    s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, RESOLVE_INDICES_KERNEL);

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;

        /* create buffer for pass context */
        if (pass->context)
            s->pass_contexts[i] = gpu_create_buffer(s->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, pass->context_size);
        else
            s->pass_contexts[i] = 0;

        /* append kernel function wrapper around the pass agent function */
        if (pass->type == TAY_PASS_SEE)
            s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, SEE_KERNEL_SOURCE, i, pass->func_name);
        else if (pass->type == TAY_PASS_ACT)
            s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, ACT_KERNEL_SOURCE, i, pass->func_name);
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

        static char pass_kernel_name[256];
        sprintf_s(pass_kernel_name, 256, "pass_%d", i);

        GpuKernel kernel = 0;

        if (pass->type == TAY_PASS_SEE) {
            TayGroup *seer_group = state->groups + pass->seer_group;
            TayGroup *seen_group = state->groups + pass->seen_group;

            int first_i = _agent_index(seen_group, s->first[pass->seen_group]);

            kernel = gpu_create_kernel(s->gpu, pass_kernel_name);
            gpu_set_kernel_buffer_argument(kernel, 0, &s->agent_buffers[pass->seer_group]);
            gpu_set_kernel_buffer_argument(kernel, 1, &s->agent_buffers[pass->seen_group]);
            gpu_set_kernel_buffer_argument(kernel, 2, &s->pass_contexts[i]);
            gpu_set_kernel_value_argument(kernel, 3, &first_i, sizeof(int));
            gpu_set_kernel_value_argument(kernel, 4, &container->dims, sizeof(int));
        }
        else if (pass->type == TAY_PASS_ACT) {
            TayGroup *act_group = state->groups + pass->act_group;

            kernel = gpu_create_kernel(s->gpu, pass_kernel_name);
            gpu_set_kernel_buffer_argument(kernel, 0, &s->agent_buffers[pass->act_group]);
            gpu_set_kernel_buffer_argument(kernel, 1, &s->pass_contexts[i]);
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
                int this_i = _agent_index(group, tag); // (int)((char *)tag - (char *)group->storage) / group->agent_size;
                int next_i = _agent_index(group, tag->next); // ? (int)((char *)tag->next - (char *)group->storage) / group->agent_size : -1;
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
        gpu_enqueue_write_buffer(s->gpu, s->pass_contexts[i], GPU_BLOCKING, pass->context_size, pass->context);
    }
}

static void _pass(TayState *state, int pass_index) {
    Space *s = state->space.storage;
    TayPass *p = state->passes + pass_index;

    TayGroup *seer_group = state->groups + p->seer_group;
    gpu_enqueue_kernel(s->gpu, s->pass_kernels[pass_index], seer_group->capacity);
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
    container->see = _pass;
    container->act = _pass;
    container->on_simulation_start = _on_simulation_start;
    container->on_run_end = _on_run_end;
}
