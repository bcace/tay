typedef struct __PACK__ Agent {
    TayAgentTag tag;
    float4 p;
    float3 v;
    float3 f;
    int seen;
} Agent;

typedef struct __PACK__ ActContext {
    int dummy;
} ActContext;

typedef struct __PACK__ SeeContext {
    float r_sq;
    float r;
    float r1;
    float r2;
    float repulsion;
    float attraction;
} SeeContext;
