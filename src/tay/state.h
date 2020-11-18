#ifndef tay_state_h
#define tay_state_h

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
    float4 radii;
    int4 max_depths;
} Tree;

typedef struct {
    Tree base;
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
    int depth_correction;
    float4 radii; /* if space is partitioned, these are suggested subdivision radii */
    SpaceType type;
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents about to be activated (inactive live agents) */
    int counts[TAY_MAX_GROUPS];
    Box box;
    union {
        CpuSimple cpu_simple;
        CpuTree cpu_tree;
    };
} Space;

typedef struct TayGroup {
    void *storage;              /* agents storage */
    struct TayAgentTag *first;  /* single linked list of available agents from storage */
    int agent_size;             /* agent size in bytes */
    int capacity;               /* max. number of agents */
    int is_point;
} TayGroup;

typedef enum TayPassType {
    TAY_PASS_SEE,
    TAY_PASS_ACT,
} TayPassType;

typedef struct TayPass {
    TayPassType type;
    union {
        int act_group;
        int seer_group;
    };
    int seen_group;
    union {
        TAY_SEE_FUNC see;
        TAY_ACT_FUNC act;
    };
    const char *func_name;
    float4 radii;
    void *context;
    int context_size;
} TayPass;

typedef enum TayStateStatus {
    TAY_STATE_STATUS_IDLE,
    TAY_STATE_STATUS_RUNNING,
} TayStateStatus;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    int passes_count;
    Space space;
    TayStateStatus running;
    const char *source;
} TayState;

void space_init(Space *space, int dims, float4 radii, int depth_correction, SpaceType init_type);
void space_add_agent(Space *space, TayAgentTag *agent, int group);
void space_on_simulation_start(struct TayState *state);
void space_run(struct TayState *state, int steps);
void space_on_simulation_end(struct TayState *state);

int group_tag_to_index(TayGroup *group, TayAgentTag *tag);

#endif
