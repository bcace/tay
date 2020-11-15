#ifndef tay_space_h
#define tay_space_h

#include "const.h"


typedef struct {
    TayAgentTag *first[TAY_MAX_THREADS];
} SimpleThread;

typedef struct {
    SimpleThread threads[TAY_MAX_GROUPS];
} CpuSimple;

typedef struct TreeCell {
    struct TreeCell *lo, *hi;           /* child partitions */
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents contained in this cell (fork or leaf) */
    int dim;                            /* dimension along which the cell's partition is divided into child partitions */
    Box box;
    float mid;
} TreeCell;

typedef struct {
    TreeCell cells[TAY_MAX_TREE_CELLS]; /* cells storage, first cell is always the root cell */
    int cells_count;
    int dims;
    int max_depth_correction;
    float4 radii;
    int4 max_depths;
} BaseTree;

typedef struct {
    BaseTree base;
} CpuTree;

typedef enum SpaceType {
    ST_ADAPTIVE,
    ST_CPU_SIMPLE,
    ST_CPU_TREE,
    ST_GPU_SIMPLE,
    ST_GPU_TREE,
} SpaceType;

typedef struct Space {
    int dims;
    SpaceType type;
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents about to be activated (inactive live agents) */
    int counts[TAY_MAX_GROUPS];
    Box box;
    union {
        CpuSimple cpu_simple;
        CpuTree cpu_tree;
    };
} Space;

void space_init(Space *space, int dims, SpaceType init_type);
void space_add_agent(Space *space, TayAgentTag *agent, int group);
void space_on_simulation_start(struct TayState *state);
void space_run(struct TayState *state, int steps);
void space_on_simulation_end(struct TayState *state);

void box_reset(Box *box, int dims);
void box_update(Box *box, float4 pos, int dims);

#endif
