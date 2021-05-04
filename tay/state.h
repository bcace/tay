#ifndef tay_state_h
#define tay_state_h

#include "tay.h"
#include "config.h"

#define TAY_MB (1 << 20)


typedef struct {
    char *agents;
    unsigned size;
    unsigned beg;
    unsigned end;
} AgentsSlice;

typedef void (*TAY_SEE_FUNC)(void *, void *, void *);
typedef void (*TAY_ACT_FUNC)(void *, void *);

typedef void (*PASS_FUNC)(struct TayPass *);
typedef void (*SEEN_FUNC)(struct TayPass *, AgentsSlice, Box, int, struct TayThreadContext *);
typedef void (*PAIRING_FUNC)(AgentsSlice, AgentsSlice, TAY_SEE_FUNC, float4, int, struct TayThreadContext *);

typedef struct {
    union {
        struct {
            int x, y, z, w;
        };
        int arr[4];
    };
} int4;

typedef struct {
    union {
        struct {
            unsigned x, y, z, w;
        };
        unsigned arr[4];
    };
} uint4;

typedef struct {
    TayAgentTag **first; /* first[thread] */
} CpuSimple;

typedef struct {
    struct TreeCell *cells; /* cells storage, first cell is always the root cell */
    unsigned max_cells; /* calculated from the size of available shared memory */
    unsigned cells_count;
    int dims; // TODO: maybe remove
    int4 max_depths;
} CpuKdTree;

typedef struct {
    struct TreeNode *nodes;
    struct TreeNode *root;
    unsigned max_nodes;
    unsigned nodes_count;
} CpuAabbTree;

typedef struct {
    struct GridCell *cells;
    struct GridCell *first_cell;
    int max_cells;
    float4 origin;
    float4 cell_sizes;
    int4 cell_counts;
} CpuGrid;

typedef struct {
    struct ZGridCell *cells;
    float4 origin;
    float4 cell_sizes;
    int4 cell_counts;
    unsigned max_cells;
    unsigned cells_count;
} CpuZGrid;

typedef struct Space {
    int dims;
    float4 radii; /* if space is partitioned, these are suggested subdivision radii */
    TaySpaceType type;
    unsigned count; /* number of agents */
    Box box;
    union {
        CpuSimple cpu_simple;
        CpuKdTree cpu_kd_tree;
        CpuAabbTree cpu_aabb_tree;
        CpuGrid cpu_grid;
        CpuZGrid cpu_z_grid;
    };
    void *shared; /* buffer shared internally by all structures in this space */
    int shared_size; /* size of the shared buffer */
} Space;

typedef struct TayGroup {
    void *agent_storage[2];
    char *storage; /* agents storage */
    char *sort_storage; /* for seen agent copies during see passes */
    int agent_size; /* agent size in bytes */
    int capacity; /* max. number of agents */
    int is_point; /* are all agents of this group points */
    Space space;
} TayGroup;

typedef enum TayPassType {
    TAY_PASS_SEE,
    TAY_PASS_ACT,
} TayPassType;

typedef struct TayPass {
    TayPassType type;
    union {
        TayGroup *act_group;
        TayGroup *seer_group;
    };
    TayGroup *seen_group;
    union {
        TAY_SEE_FUNC see;
        TAY_ACT_FUNC act;
    };
    float4 radii;
    int self_see;
    void *context;
    /* data prepared by the compile step */
    PASS_FUNC struct_pass_func;
    SEEN_FUNC seen_func;
    PAIRING_FUNC pairing_func;
} TayPass;

typedef enum TayStateStatus {
    TAY_STATE_STATUS_IDLE,
    TAY_STATE_STATUS_RUNNING,
} TayStateStatus;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    int passes_count;
    TayStateStatus running;
    TayError error;
    double ms_per_step; /* for the last run */
} TayState;

void state_set_error(TayState *state, TayError error);

int group_is_active(TayGroup *group);
int group_is_inactive(TayGroup *group);

#endif
