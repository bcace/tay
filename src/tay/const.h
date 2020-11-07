#ifndef tay_const_h
#define tay_const_h

#define TAY_MAX_GROUPS          8
#define TAY_MAX_PASSES          32
#define TAY_INSTRUMENT          0
#define TAY_MAX_AGENTS          1000000
#define TAY_GPU_DEAD            0xffffffffffffffff

#define TAY_AGENT_POSITION(__agent_tag__) (*(float4 *)(__agent_tag__ + 1))


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

#endif
