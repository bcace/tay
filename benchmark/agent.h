#ifndef tay_agent_h
#define tay_agent_h

#include "tay.h"


#pragma pack(push, 1)

typedef struct Agent {
    TayAgentTag tag;
    float4 p;
    float4 v;
    float4 b_buffer;
    int b_buffer_count;
    float4 f_buffer;
} Agent;

typedef struct ActContext {
    float4 min;
    float4 max;
} ActContext;

typedef struct SeeContext {
    float4 radii;
} SeeContext;

typedef struct BoxAgent {
    TayAgentTag tag;
    float4 min;
    float4 max;
    float4 v;
    float4 b_buffer;
    int b_buffer_count;
    float4 f_buffer;
} BoxAgent;

#pragma pack(pop)

void agent_see(Agent *a, Agent *b, SeeContext *context);
void agent_act(Agent *agent, ActContext *context);

void box_agent_see(BoxAgent *a, BoxAgent *b, SeeContext *context);
void box_agent_act(BoxAgent *agent, ActContext *context);

#endif
