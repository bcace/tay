#ifndef tay_state_h
#define tay_state_h

#include "tay.h"
#include "config.h"

#define TAY_MB (1 << 20)

typedef void (*TAY_SEE_FUNC)(void *, void *, void *);
typedef void (*TAY_ACT_FUNC)(void *, void *);

typedef void (*SEE_PAIRING_FUNC)(TayAgentTag *, TayAgentTag *, TAY_SEE_FUNC, float4, int, struct TayThreadContext *);


typedef struct {
    union {
        struct {
            int x, y, z, w;
        };
        int arr[4];
    };
} int4;

typedef struct {
    TayAgentTag **first; /* first_ptrs[thread][group] */
} CpuSimple;

typedef struct {
    struct TreeCell *cells; /* cells storage, first cell is always the root cell */
    int max_cells; /* calculated from the size of available shared memory */
    int cells_count;
    int dims;
    int4 max_depths;
} CpuKdTree;

typedef struct {
    struct TreeNode *nodes;
    struct TreeNode *root;
    int max_nodes;
    int nodes_count;
} CpuAabbTree;

typedef struct {
    struct Bin *bins; /* bins storage */
    struct Bin *first_bin; /* first bin that contains any agents */
    float4 cell_sizes;
    float4 grid_origin;
    unsigned modulo_mask;
} CpuHashGrid;

typedef struct Space {
    int dims;
    float4 radii; /* if space is partitioned, these are suggested subdivision radii */
    TaySpaceType type;
    TayAgentTag *first; /* unsorted agents */
    int count; /* counts of unsorted agents */
    Box box;
    union {
        CpuSimple cpu_simple;
        CpuKdTree cpu_tree;
        CpuHashGrid cpu_grid;
        CpuAabbTree cpu_aabb_tree;
    };
    void *shared; /* buffer shared internally by all structures in this space */
    int shared_size; /* size of the shared buffer */
} Space;

typedef struct TayGroup {
    void *storage; /* agents storage */
    struct TayAgentTag *first; /* single linked list of available agents from storage */
    int agent_size; /* agent size in bytes */
    int capacity; /* max. number of agents */
    int is_point; /* are all agents of this group points */
    Space space; // TODO: rename to space once I finish the transition
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
    void *context;
    /* data prepared by the compile step */
    union {
        Space *act_space;
        Space *seer_space;
    };
    Space *seen_space;
    SEE_PAIRING_FUNC pairing_func;
    void (*exec_func)(struct TayPass *pass);
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
