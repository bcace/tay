#ifndef tay_agent_h
#define tay_agent_h

#include "tay.h"


#pragma pack(push, 1)

typedef struct Agent {
    TayAgentTag tag;
    float4 p;
    float3 v;
    float3 f;
} Agent;

typedef struct ActContext {
    int dummy;
} ActContext;

typedef struct SeeContext {
    float r_sq;
    float r;
    float r1;
    float r2;
    float repulsion;
    float attraction;
} SeeContext;

void agent_see(Agent *a, Agent *b, SeeContext *context);
void agent_act(Agent *agent, ActContext *context);


float3 float3_null();
float3 float3_make(float x, float y, float z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_div_scalar(float3 a, float s);
float3 float3_mul_scalar(float3 a, float s);
float3 float3_normalize(float3 a);
float3 float3_normalize_t(float3 a, float b);
float float3_length(float3 a);

float4 float4_null();
float4 float4_make(float x, float y, float z, float w);
float4 float4_add(float4 a, float4 b);
float4 float4_sub(float4 a, float4 b);
float4 float4_div_scalar(float4 a, float s);

extern const char *agent_kernels_source;

#pragma pack(pop)

#endif
