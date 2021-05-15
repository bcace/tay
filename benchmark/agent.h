
typedef struct __attribute__((packed)) Agent {
    TayAgentTag tag;
    float4 p;
    float4 v;
    float4 b_buffer;
    int b_buffer_count;
    float4 f_buffer;
    int result_index;
} Agent;

typedef struct __attribute__((packed)) ActContext {
    float4 min;
    float4 max;
} ActContext;

typedef struct __attribute__((packed)) SeeContext {
    float4 radii;
} SeeContext;

typedef struct __attribute__((packed)) BoxAgent {
    TayAgentTag tag;
    float4 min;
    float4 max;
    float4 v;
    float4 b_buffer;
    int b_buffer_count;
    float4 f_buffer;
    int result_index;
} BoxAgent;

void agent_see(global Agent *a, global Agent *b, constant SeeContext *context);
void agent_act(global Agent *agent, constant ActContext *context);

void box_agent_see(global BoxAgent *a, global BoxAgent *b, constant SeeContext *context);
void box_agent_act(global BoxAgent *agent, constant ActContext *context);
