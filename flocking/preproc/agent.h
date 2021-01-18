typedef struct __PACK__ Agent {
    TayAgentTag tag;
    float4 p;
    float3 dir;
    float speed;
    float3 separation;
    float3 alignment;
    float3 cohesion;
} Agent;

typedef struct __PACK__ ActContext {
    int dummy;
} ActContext;

typedef struct __PACK__ SeeContext {
    float r_sq;
    float r;
} SeeContext;
