typedef struct __PACK__ Agent {
    TayAgentTag tag;
    float4 p;
    float4 v;
} Agent;

typedef struct __PACK__ ActContext {
    int dummy;
} ActContext;

typedef struct __PACK__ SeeContext {
    float4 radii;
} SeeContext;
