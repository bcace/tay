typedef struct __PACK__ Agent {
    TayAgentTag tag;
    float4 p;
    float3 v;
    float3 separation;
    float3 cohesion;
    float3 alignment;
} Agent;

typedef struct __PACK__ ActContext {
    int dummy;
} ActContext;

typedef struct __PACK__ SeeContext {
    float4 radii_sq;
} SeeContext;
