#ifndef tay_state_h
#define tay_state_h

#include "tay.h"
#include "config.h"
#include "ocl.h"

#define TAY_MB (1 << 20)
#define TAY_MAX_FUNC_NAME 512


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

typedef struct {
    void *agent_buffer;
    void *agent_sort_buffer;
    void *space_buffer;
    unsigned push_agents; /* set this flag if agents have to be pushed to gpu before a run */
} OclCommon;

typedef struct TayPicGrid {
    char *node_storage;
    unsigned nodes_count;
    unsigned nodes_capacity;
    unsigned node_size; /* user-defined node struct size in bytes */
    float cell_size;
    int dims;
    /* calculated later at each run step */
    float4 origin;
    uint4 node_counts;
} TayPicGrid;

typedef struct Space {
    int dims;
    float4 min_part_sizes; /* if space is partitioned, these are suggested subdivision sizes */
    TaySpaceType type;
    unsigned count; /* number of agents */
    Box box;
    int is_box_fixed;
    union {
        CpuSimple cpu_simple;
        CpuKdTree cpu_kd_tree;
        CpuAabbTree cpu_aabb_tree;
        CpuGrid cpu_grid;
        CpuZGrid cpu_z_grid;
        OclCommon ocl_common;
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
    unsigned id;
    int ocl_enabled; /* false both if ocl code is excluded from compilation or creating the ocl context failed */
} TayGroup;

typedef enum TayPassType {
    TAY_PASS_SEE,
    TAY_PASS_ACT,
} TayPassType;

typedef struct TayPass {
    TayPassType type;
    int is_pic;
    union {
        TayGroup *act_group;
        TayGroup *seer_group;
        TayGroup *pic_group;
    };
    union {
        TayGroup *seen_group;
        TayPicGrid *pic;
    };
    union {
        TAY_SEE_FUNC see;
        TAY_ACT_FUNC act;
    };
    union {
        struct { /* agent see */
            float4 radii;
            int self_see;
        };
        struct { /* pic see */
            unsigned kernel_size;
        };
    };
    void *context;
    /* ocl-specific data */
    char func_name[TAY_MAX_FUNC_NAME];
    unsigned context_size;
    /* data prepared by the compile step */
    struct { /* cpu pass */
        SEEN_FUNC seen_func;
        PAIRING_FUNC pairing_func;
    };
    struct { /* ocl pass */
        void *context_buffer;
        void *pass_kernel;
    };
} TayPass;

typedef enum TayStateStatus {
    TAY_STATE_STATUS_IDLE,
    TAY_STATE_STATUS_RUNNING,
} TayStateStatus;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    unsigned passes_count;
    TayPicGrid pics[TAY_MAX_PICS];
    unsigned pics_count;
    TayStateStatus running;
    TayError error;
    double ms_per_step; /* for the last run */
    TayOcl ocl;
    unsigned next_group_id;
} TayState;

void tay_set_error(TayState *state, TayError error);
void tay_set_error2(TayState *state, TayError error, const char *message);

int group_is_active(TayGroup *group);
int group_is_inactive(TayGroup *group);
int pass_is_ocl_enabled(TayPass *pass);
int pic_is_active(TayPicGrid *pic);

int state_compile(TayState *state);

int pic_prepare_grids(TayState *state);
void pic_run_see_pass(TayPass *pass);
void pic_run_act_pass(TayPass *pass);

#endif
