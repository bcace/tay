#include "state.h"
#include "gpu.h"
#include <stdlib.h>
#include <assert.h>

typedef int cl_int;


const cl_int NULL_AGENT = -1;

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
} Space;

static Space *_init() {
    Space *s = malloc(sizeof(Space));
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        s->first[i] = NULL_AGENT;
    s->gpu = gpu_create();
    return s;
}

static void _destroy(TaySpace *space) {
    Space *s = (Space *)space;
    gpu_destroy(s->gpu);
    free(s);
}

static void _add(TaySpace *space, TayAgentTag *agent, int group, int index) {
    Space *s = (Space *)space;
    Tag *tag = (Tag *)agent;
    tag->next = s->first[group];
    s->first[group] = index;
}

static void _on_simulation_start(TaySpace *space, TayState *state) {
}

static void _on_simulation_end(TaySpace *space) {
}

static void _see(TaySpace *space, TayPass *pass) {
}

static void _act(TaySpace *space, TayPass *pass) {
}

static void _update(TaySpace *space) {
}

static void _release(TaySpace *space) {
}

void space_gpu_simple_init(TaySpace *space, int dims) {
    assert(sizeof(Tag) == TAY_AGENT_TAG_SIZE);
    space_init(space, _init(), dims, _destroy, _add, _see, _act, _update, _on_simulation_start, _on_simulation_end);
}
