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

void agent_see(Agent *a, Agent *b, SeeContext *context);
void agent_act(Agent *agent, ActContext *context);

float4 float4_null();
float4 float4_make(float x, float y, float z);
float4 float4_add(float4 a, float4 b);
float4 float4_sub(float4 a, float4 b);
float4 float4_div_scalar(float4 a, float s);

extern const char *agent_kernels_source;

#pragma pack(pop)

#endif
