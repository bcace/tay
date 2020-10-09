#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>


typedef struct {
    int first[TAY_MAX_GROUPS];
} Cell;

typedef struct {
    Cell *cells;
    int cells_cap;
    int dims;
    float radii[TAY_MAX_DIMENSIONS];
} Grid;

static Grid *_init(int dims, float *radii) {
    Grid *g = malloc(sizeof(Grid));
    g->cells_cap = 1000000;
    g->cells = malloc(sizeof(Cell) * g->cells_cap);
    g->dims = dims;
    for (int i = 0; i < dims; ++i)
        g->radii[i] = radii[i];
    return g;
}

static void _destroy(TaySpace *space) {
    Grid *g = space->storage;
    free(g->cells);
    free(g);
}

static void _add(TaySpace *space, TayAgentTag *agent, int group) {

}

static void _see(TaySpace *space, TayPass *pass) {

}

static void _act(TaySpace *space, TayPass *pass) {

}

static void _update(TaySpace *space) {

}

void grid_init(TaySpace *space, int dims, float *radii) {
    space_init(space, _init(dims, radii), dims, _destroy, _add, _see, _act, _update);
}
