#ifndef tay_state_h
#define tay_state_h

#include "tay.h"
#include "const.h"

#define TAY_CPU_SHARED_TEMP_ARENA_SIZE      (TAY_MAX_AGENTS * sizeof(float4) * 2)
#define TAY_CPU_SHARED_CELL_ARENA_SIZE      (TAY_MAX_CELLS * 200)
#define TAY_CPU_SHARED_THREAD_ARENA_SIZE    20000000


typedef struct {
    TayAgentTag *first[TAY_MAX_THREADS];
} SimpleThread;

typedef struct {
    SimpleThread threads[TAY_MAX_GROUPS];
} CpuSimple;

typedef struct {
    struct TreeCell *cells; /* cells storage, first cell is always the root cell */
    int cells_count;
    int dims;
    float4 radii;
    int4 max_depths;
} Tree;

typedef struct {
    Tree base;
} CpuTree;

typedef struct {
    struct Bucket *buckets; /* buckets storage */
    struct Bucket *first_bucket; /* first bucket that contains any agents */
    int4 cell_counts;
    float4 cell_sizes;
    float4 grid_origin;
} CpuGrid;

typedef struct {
    void *pass_kernels[TAY_MAX_PASSES];
    void *pass_kernels_indirect[TAY_MAX_PASSES];
} GpuSimple;

typedef struct {
    Tree base;
    void *pass_kernels[TAY_MAX_PASSES];
    void *resolve_cell_pointers_kernel;
    void *resolve_cell_agent_pointers_kernel;
} GpuTree;

typedef struct {
    struct GpuContext *gpu;
    void *agent_io_buffer; // equivalent to temp_arena
    void *cells_buffer; // equivalent to cell arena
    void *agent_buffers[TAY_MAX_GROUPS];
    void *pass_context_buffers[TAY_MAX_PASSES];
    void *resolve_pointers_kernel;
    void *fetch_new_positions_kernel;
    char text[TAY_GPU_MAX_TEXT_SIZE];
    int text_size;
} GpuShared;

typedef struct {
    char temp_arena[TAY_CPU_SHARED_TEMP_ARENA_SIZE];
    char cell_arena[TAY_CPU_SHARED_CELL_ARENA_SIZE];
    char thread_arena[TAY_CPU_SHARED_THREAD_ARENA_SIZE];
} CpuShared;

typedef enum SpaceType {
    ST_NONE =                   0x0000,
    ST_CPU =                    0x0001,
    ST_GPU =                    0x0002,
    ST_ADAPTIVE =               0x0010,
    ST_SIMPLE =                 0x0020,
    ST_TREE =                   0x0040,
    ST_GRID =                   0x0080,
    ST_FINAL =                  0x1000,
    ST_GPU_SIMPLE =             (ST_GPU | ST_SIMPLE),

    ST_CPU_SIMPLE =             (ST_FINAL | ST_CPU | ST_SIMPLE),
    ST_CPU_TREE =               (ST_FINAL | ST_CPU | ST_TREE),
    ST_CPU_GRID =               (ST_FINAL | ST_CPU | ST_GRID),
    ST_GPU_SIMPLE_DIRECT =      (ST_FINAL | 0x0100 | ST_GPU_SIMPLE), /* only used when agent populations don't change */
    ST_GPU_SIMPLE_INDIRECT =    (ST_FINAL | 0x0200 | ST_GPU_SIMPLE),
    ST_GPU_TREE =               (ST_FINAL | ST_GPU | ST_TREE),
    ST_CYCLE_ALL =              (ST_FINAL | 0x0ff0),
} SpaceType;

typedef struct Space {
    int dims;
    int depth_correction;
    float4 radii; /* if space is partitioned, these are suggested subdivision radii */
    SpaceType type;
    TayAgentTag *first[TAY_MAX_GROUPS]; /* agents about to be activated (inactive live agents) */
    int counts[TAY_MAX_GROUPS];
    Box box;
    CpuSimple cpu_simple;
    CpuTree cpu_tree;
    CpuGrid cpu_grid;
    CpuShared cpu_shared;
    GpuSimple gpu_simple;
    GpuTree gpu_tree;
    GpuShared gpu_shared;
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

void space_init(Space *space, int dims, float4 radii);
void space_release(Space *space);
void space_add_agent(Space *space, TayAgentTag *agent, int group);
void space_on_simulation_start(TayState *state);
void space_run(TayState *state, int steps, SpaceType space_type, int depth_correction);
void space_on_simulation_end(TayState *state);
void *space_get_temp_arena(Space *space, int size);
void *space_get_cell_arena(Space *space, int size, int zero);
int space_get_thread_mem_size();
void *space_get_thread_mem(Space *space, int thread_i);

#endif
