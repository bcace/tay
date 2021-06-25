
typedef struct __attribute__((packed)) Agent {
    TayAgentTag tag;
    float4 p;
    float4 dir;
    float speed;
    float4 separation;
    float4 alignment;
    float4 cohesion;
    int cohesion_count;
    int separation_count;
} Agent;

typedef struct __attribute__((packed)) ActContext {
    int dummy;
} ActContext;

typedef struct __attribute__((packed)) SeeContext {
    float r;
    float separation_r;
} SeeContext;

void agent_see(global Agent *a, global Agent *b, constant SeeContext *context);
void agent_act(global Agent *a, constant ActContext *context);
