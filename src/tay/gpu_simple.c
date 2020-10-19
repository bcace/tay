#include "state.h"
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MAX_KERNEL_SOURCE_LENGTH 10000

typedef int cl_int;


const cl_int NULL_AGENT = -1;

const char *SEE_KERNEL_SOURCE = "kernel void pass_%d(global Agent *seer_agents, global Agent *seen_agents, global SeeContext *see_context, int first_seen, int dims) {\n\
    global Agent *seer_agent = seer_agents + get_global_id(0);\n\
    for (int i = first_seen; i != -1; i = seen_agents[i].tag.next) {\n\
        global Agent *seen_agent = seen_agents + i;\n\
        \n\
        if (seer_agent == seen_agent)\n\
            goto NARROW_PHASE_NEGATIVE;\n\
        \n\
        float4 d = seer_agent->p - seen_agent->p;\n\
        if (d.x > see_context->radii.x || d.x < -see_context->radii.x)\n\
            goto NARROW_PHASE_NEGATIVE;\n\
        if (dims > 1) {\n\
            if (d.y > see_context->radii.y || d.y < -see_context->radii.y)\n\
                goto NARROW_PHASE_NEGATIVE;\n\
        }\n\
        if (dims > 2) {\n\
            if (d.z > see_context->radii.z || d.z < -see_context->radii.z)\n\
                goto NARROW_PHASE_NEGATIVE;\n\
        }\n\
        if (dims > 3) {\n\
            if (d.w > see_context->radii.w || d.w < -see_context->radii.w)\n\
                goto NARROW_PHASE_NEGATIVE;\n\
        }\n\
        \n\
        %s(seer_agent, seen_agent, see_context);\n\
        NARROW_PHASE_NEGATIVE:;\n\
    }\n\
}\n";

const char *ACT_KERNEL_SOURCE = "kernel void pass_%d(global Agent *agents, global ActContext *act_context) {\n\
    %s(agents + get_global_id(0), act_context);\n\
}\n";

#pragma pack(push, 1)
typedef struct {
    cl_int next;
    cl_int padding; /* size of this struct has to be equal to TAY_AGENT_TAG_SIZE */
} Tag;
#pragma pack(pop)

typedef struct {
    cl_int first[TAY_MAX_GROUPS];
    GpuContext *gpu;
    GpuBuffer agent_buffers[TAY_MAX_GROUPS];
    GpuBuffer pass_contexts[TAY_MAX_PASSES];
    GpuKernel pass_kernels[TAY_MAX_PASSES];
    char kernels_source[MAX_KERNEL_SOURCE_LENGTH];
    int bytes_written;
} Space;

static Space *_init() {
    Space *s = malloc(sizeof(Space));
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        s->first[i] = NULL_AGENT;
    s->gpu = gpu_create();
    s->kernels_source[0] = '\0';
    s->bytes_written = 0;
    return s;
}

static void _destroy(TaySpaceContainer *container) {
    Space *s = (Space *)container->storage;
    gpu_destroy(s->gpu);
    free(s);
}

static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
    Space *s = (Space *)container->storage;
    Tag *tag = (Tag *)agent;
    tag->next = s->first[group];
    s->first[group] = index;
}

static void _on_simulation_start(TaySpaceContainer *container, TayState *state) {
    Space *s = (Space *)container->storage;

    /* create agent buffers */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            s->agent_buffers[i] = gpu_create_buffer(s->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, group->capacity * group->agent_size);
    }

    /* copy agent structs and behaviors code into the buffer */
    s->bytes_written += sprintf_s(s->kernels_source, MAX_KERNEL_SOURCE_LENGTH, state->source);

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
            assert(0); /* not implemented */

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

            kernel = gpu_create_kernel(s->gpu, pass_kernel_name);
            gpu_set_kernel_buffer_argument(kernel, 0, &s->agent_buffers[pass->seer_group]);
            gpu_set_kernel_buffer_argument(kernel, 1, &s->agent_buffers[pass->seen_group]);
            gpu_set_kernel_buffer_argument(kernel, 2, &s->pass_contexts[i]);
            gpu_set_kernel_value_argument(kernel, 3, &s->first[pass->seen_group], sizeof(cl_int));
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

    /* enqueue writing agents to GPU */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            gpu_enqueue_write_buffer(s->gpu, s->agent_buffers[i], GPU_BLOCKING, group->capacity * group->agent_size, group->storage);
    }

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
    assert(sizeof(Tag) == TAY_AGENT_TAG_SIZE);
    space_container_init(container, _init(), dims, _destroy);
    container->add = _add;
    container->see = _pass;
    container->act = _pass;
    container->on_simulation_start = _on_simulation_start;
    container->on_run_end = _on_run_end;
}
