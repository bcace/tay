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

void agent_see(Agent *a, Agent *b, SeeContext *context);
void agent_act(Agent *agent, ActContext *context);

void box_agent_see(BoxAgent *a, BoxAgent *b, SeeContext *context);
void box_agent_act(BoxAgent *agent, ActContext *context);


float3 float3_null();
float3 float3_make(float x, float y, float z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_div_scalar(float3 a, float s);
float3 float3_mul_scalar(float3 a, float s);
float3 float3_normalize(float3 a);
float3 float3_normalize_to(float3 a, float b);
float float3_length(float3 a);
float float3_dot(float3 a, float3 b);

float4 float4_null();
float4 float4_make(float x, float y, float z, float w);
float4 float4_add(float4 a, float4 b);
float4 float4_sub(float4 a, float4 b);
float4 float4_div_scalar(float4 a, float s);

extern const char *agent_kernels_source;

#pragma pack(pop)

#endif
