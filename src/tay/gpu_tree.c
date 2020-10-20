#include "state.h"
#include "gpu.h"
#include <stdlib.h>
#include <assert.h>


#pragma pack(push, 1)

typedef struct {
    int next;
    int padding;
} Tag;

typedef struct {
    int first[TAY_MAX_GROUPS];
} Cell;

#pragma pack(pop)

typedef struct {
    int first[TAY_MAX_GROUPS]; /* agents to be added to the space at next update */
    Cell *cells;
    int max_cells;
    int available_cell;
} Space;

static Space *_init() {
    Space *s = malloc(sizeof(Space));
    s->max_cells = 100000;
    s->cells = malloc(sizeof(Cell) * s->max_cells);
    s->available_cell = 1; /* first cell is always the root cell */
    return s;
}

static void _destroy(TaySpaceContainer *container) {
    Space *s = container->storage;
    free(s->cells);
    free(s);
}

void space_gpu_tree_init(TaySpaceContainer *container, int dims) {
    assert(sizeof(Tag) == TAY_AGENT_TAG_SIZE);
    space_container_init(container, _init(), dims, _destroy);
}
