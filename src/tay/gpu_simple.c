#include "state.h"
#include "gpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define MAX_KERNEL_SOURCE_LENGTH 100024

typedef int cl_int;


const cl_int NULL_AGENT = -1;

const char *SEE_KERNEL_SOURCE = "kernel void pass_%d(global Agent *agents, global SeeContext *see_context) {\n\
    int count = get_global_size(0);\n\
    global Agent *a = agents + get_global_id(0);\n\
    for (int i = 0; i < count; ++i)\n\
        %s(a, agents + i, see_context);\n\
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

    s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, state->source);

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;

        if (pass->context)
            s->pass_contexts[i] = gpu_create_buffer(s->gpu, GPU_MEM_READ_ONLY, GPU_MEM_NONE, pass->context_size);
        else
            s->pass_contexts[i] = 0;

        if (pass->type == TAY_PASS_SEE)
            s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, SEE_KERNEL_SOURCE, i, pass->func_name);
        else if (pass->type == TAY_PASS_ACT)
            s->bytes_written += sprintf_s(s->kernels_source + s->bytes_written, MAX_KERNEL_SOURCE_LENGTH - s->bytes_written, ACT_KERNEL_SOURCE, i, pass->func_name);
        else
            assert(0); /* not implemented */

        assert(s->bytes_written < MAX_KERNEL_SOURCE_LENGTH);
    }

    assert(state->source != 0);
    gpu_build_program(s->gpu, s->kernels_source);

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage)
            s->agent_buffers[i] = gpu_create_buffer(s->gpu, GPU_MEM_READ_AND_WRITE, GPU_MEM_NONE, group->capacity * group->agent_size_with_tag);
    }
}

static void _on_simulation_end(TaySpaceContainer *container) {
}

static void _see(TaySpaceContainer *container, TayPass *pass) {
}

static void _act(TaySpaceContainer *container, TayPass *pass) {
}

static void _update(TaySpaceContainer *container) {
}

static void _release(TaySpaceContainer *container) {
}

void space_gpu_simple_init(TaySpaceContainer *container, int dims) {
    assert(sizeof(Tag) == TAY_AGENT_TAG_SIZE);
    space_container_init(container, _init(), dims, _destroy, _add, _see, _act, _update, _on_simulation_start, _on_simulation_end);
}
