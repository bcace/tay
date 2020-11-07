#ifndef tay_tree_h
#define tay_tree_h

#include "const.h"


typedef struct {
    union {
        struct {
            int x, y, z, w;
        };
        int arr[4];
    };
} int4;

typedef struct Cell {
    struct Cell *lo, *hi;                       /* child partitions */
    struct TayAgentTag *first[TAY_MAX_GROUPS];  /* agents contained in this cell (fork or leaf) */
    int dim;                                    /* dimension along which the cell's partition is divided into child partitions */
    Box box;
    float mid;
} Cell;

typedef struct {
    struct TayAgentTag *first[TAY_MAX_GROUPS];  /* agents are kept here while not in cells, will be sorted into the tree at next step */
    Cell *cells;                                /* cells storage, first cell is always the root cell */
    int max_cells;
    int cells_count;
    int dims;
    int max_depth_correction;
    float4 radii;
    int4 max_depths;
    Box box;
} TreeBase;

void tree_reset_box(Box *box, int dims);
void tree_update_box(Box *box, float4 p, int dims);

void tree_clear_cell(Cell *cell);

void tree_init(TreeBase *base, int dims, float4 radii, int max_depth_correction);
void tree_destroy(TreeBase *base);
void tree_clear(TreeBase *tree); /* NOTE: this does not return contained agents into storage */
void tree_add(TreeBase *base, TayAgentTag *agent, int group);
void tree_update(TreeBase *base);

#endif
