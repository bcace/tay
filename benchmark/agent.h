#ifndef tay_agent_h
#define tay_agent_h

#include "tay.h"


#pragma pack(push, 1)

typedef struct Agent {
    TayAgentTag tag;
    float4 p;
    float3 v;
    float3 b_buffer;
    int b_buffer_count;
    float3 f_buffer;
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


float2 float2_get_agent_position(void *agent);
float3 float3_get_agent_position(void *agent);
float4 float4_get_agent_position(void *agent);

void float2_set_agent_position(void *agent, float2 p);
void float3_set_agent_position(void *agent, float3 p);
void float4_set_agent_position(void *agent, float4 p);

float3 float3_null();
float3 float3_make(float x, float y, float z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_div_scalar(float3 a, float s);

extern const char *agent_kernels_source;

#pragma pack(pop)

#endif
