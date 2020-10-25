#ifndef tay_tree_h
#define tay_tree_h

#include "state.h"


typedef struct {
    float min[TAY_MAX_DIMENSIONS];
    float max[TAY_MAX_DIMENSIONS];
} Box;

typedef struct Cell {
    struct Cell *lo, *hi;               /* child partitions */
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents contained in this cell (fork or leaf) */
    int dim;                            /* dimension along which the cell's partition is divided into child partitions */
    Box box;
} Cell;

typedef struct {
    unsigned char dims[TAY_MAX_DIMENSIONS];
} Depths;

typedef struct {
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents are kept here while not in cells, will be sorted into the tree at next step */
    Cell *cells;                        /* cells storage, first cell is always the root cell */
    int max_cells;
    int available_cell;
    int dims;
    int max_depth_correction;
    float radii[TAY_MAX_DIMENSIONS];
    int max_depths[TAY_MAX_DIMENSIONS];
    Box box;
} Space;

void tree_reset_box(Box *box, int dims);
void tree_update_box(Box *box, float *p, int dims);

void tree_clear_cell(Cell *cell);

Space *tree_init(int dims, float *radii, int max_depth_correction);
void tree_destroy(Space *space);
void tree_add(Space *space, TayAgentTag *agent, int group);
void tree_update(Space *space);

#endif
