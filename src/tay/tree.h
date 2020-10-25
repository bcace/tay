#ifndef tay_tree_h
#define tay_tree_h

// TODO: maybe move defines into const and include that
#include "state.h"


#pragma pack(push, 1)
typedef struct {
    float min[TAY_MAX_DIMENSIONS];
    float max[TAY_MAX_DIMENSIONS];
} Box;
#pragma pack(pop)

typedef struct Cell {
    struct Cell *lo, *hi;                       /* child partitions */
    struct TayAgentTag *first[TAY_MAX_GROUPS];  /* agents contained in this cell (fork or leaf) */
    int dim;                                    /* dimension along which the cell's partition is divided into child partitions */
    Box box;
} Cell;

typedef struct {
    struct TayAgentTag *first[TAY_MAX_GROUPS];  /* agents are kept here while not in cells, will be sorted into the tree at next step */
    Cell *cells;                                /* cells storage, first cell is always the root cell */
    int max_cells;
    int available_cell;
    int dims;
    int max_depth_correction;
    float radii[TAY_MAX_DIMENSIONS];
    int max_depths[TAY_MAX_DIMENSIONS];
    Box box;
} TreeBase;

void tree_reset_box(Box *box, int dims);
void tree_update_box(Box *box, float *p, int dims);

void tree_clear_cell(Cell *cell);

void tree_init(TreeBase *base, int dims, float *radii, int max_depth_correction);
void tree_destroy(TreeBase *base);
void tree_add(TreeBase *base, TayAgentTag *agent, int group);
void tree_update(TreeBase *base);

#endif
