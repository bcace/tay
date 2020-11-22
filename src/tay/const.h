#ifndef tay_const_h
#define tay_const_h

#define TAY_MAX_GROUPS          8
#define TAY_MAX_PASSES          32
#define TAY_MAX_THREADS         64
#define TAY_INSTRUMENT          0
#define TAY_MAX_AGENTS          1000000
// #define TAY_MAX_TREE_CELLS      100000
// TODO: rename to TAY_GPU_DEAD_ADDR and move it to space.h
#define TAY_GPU_DEAD            0xffffffffffffffff
#define TAY_GPU_MAX_TEXT_SIZE   10000
#define TAY_SPACE_SHARED_SIZE   (TAY_MAX_AGENTS * sizeof(float4))
#define TAY_SPACE_CELL_ARENA_SIZE 10000000

#define TAY_AGENT_POSITION(__agent_tag__) (*(float4 *)((TayAgentTag *)(__agent_tag__) + 1))


typedef struct {
    union {
        struct {
            int x, y, z, w;
        };
        int arr[4];
    };
} int4;

#pragma pack(push, 1)
typedef struct float4 {
    union {
        struct {
            float x, y, z, w;
        };
        float arr[4];
    };
} float4;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    float4 min;
    float4 max;
} Box;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct TayAgentTag {
    struct TayAgentTag *next;
} TayAgentTag;
#pragma pack(pop)

typedef void (*TAY_SEE_FUNC)(void *, void *, void *);
typedef void (*TAY_ACT_FUNC)(void *, void *);

#endif
