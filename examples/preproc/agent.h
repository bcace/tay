typedef struct __PACK__ Agent {
    TayAgentTag tag;
    float4 p;
    float3 dir;
    float speed;
    float3 separation;
    float3 alignment;
    float3 cohesion;
    int cohesion_count;
    int separation_count;
} Agent;

typedef struct __PACK__ ActContext {
    int dummy;
} ActContext;

typedef struct __PACK__ SeeContext {
    float r;
    float separation_r;
} SeeContext;

void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *context);
void agent_act(__GLOBAL__ Agent *a, __GLOBAL__ ActContext *context);
