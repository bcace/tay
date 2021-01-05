#ifndef tay_agent_h
#define tay_agent_h

#include "tay.h"


#pragma pack(push, 1)

typedef struct Agent {
    TayAgentTag tag;
    float4 p;
    float3 v;
    float3 separation;
    float3 cohesion;
    float3 alignment;
} Agent;

typedef struct ActContext {
    int dummy;
} ActContext;

typedef struct SeeContext {
    float4 radii_sq;
} SeeContext;

void agent_see(Agent *a, Agent *b, SeeContext *context);
void agent_act(Agent *agent, ActContext *context);


float3 float3_null();
float3 float3_make(float x, float y, float z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_div_scalar(float3 a, float s);

float4 float4_null();
float4 float4_make(float x, float y, float z, float w);
float4 float4_add(float4 a, float4 b);
float4 float4_sub(float4 a, float4 b);
float4 float4_div_scalar(float4 a, float s);

extern const char *agent_kernels_source;

#pragma pack(pop)

#endif
