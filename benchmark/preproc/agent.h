typedef struct __PACK__ Agent {
    TayAgentTag tag;
    float4 p;
    float4 v;
    float4 b_buffer;
    int b_buffer_count;
    float4 f_buffer;
} Agent;

typedef struct __PACK__ ActContext {
    float4 min;
    float4 max;
} ActContext;

typedef struct __PACK__ SeeContext {
    float4 radii;
} SeeContext;

typedef struct __PACK__ BoxAgent {
    TayAgentTag tag;
    float4 min;
    float4 max;
    float4 v;
    float4 b_buffer;
    int b_buffer_count;
    float4 f_buffer;
} BoxAgent;

void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *context);
void agent_act(__GLOBAL__ Agent *agent, __GLOBAL__ ActContext *context);

void box_agent_see(__GLOBAL__ BoxAgent *a, __GLOBAL__ BoxAgent *b, __GLOBAL__ SeeContext *context);
void box_agent_act(__GLOBAL__ BoxAgent *agent, __GLOBAL__ ActContext *context);
