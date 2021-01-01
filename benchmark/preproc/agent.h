typedef struct __PACK__ Agent {
    TayAgentTag tag;
    float4 p;
    float3 v;
    float3 b_buffer;
    int b_buffer_count;
    float3 f_buffer;
} Agent;

typedef struct __PACK__ ActContext {
    float4 min;
    float4 max;
} ActContext;

typedef struct __PACK__ SeeContext {
    float4 radii;
} SeeContext;
