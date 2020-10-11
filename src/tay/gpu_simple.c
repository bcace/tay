#include "state.h"
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
} Space;

static Space *_init() {
    Space *s = malloc(sizeof(Space));
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        s->first[i] = NULL_AGENT;
    return s;
}

static void _destroy(TaySpace *space) {
    free(space->storage);
}

static void _add(TaySpace *space, TayAgentTag *agent, int group, int index) {
    Space *s = (Space *)space;
    Tag *tag = (Tag *)agent;
    tag->next = s->first[group];
    s->first[group] = index;
}

static void _see(TaySpace *space, TayPass *pass) {
}

static void _act(TaySpace *space, TayPass *pass) {
}

static void _update(TaySpace *space) {
}

void space_gpu_simple_init(TaySpace *space, int dims) {
    assert(sizeof(Tag) == TAY_AGENT_TAG_SIZE);
    space_init(space, _init(), dims, _destroy, _add, _see, _act, _update);
}
