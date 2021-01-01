#include "agent.h"


void agent_see(Agent *a, Agent *b, SeeContext *c) {
    float3 a_p = float3_get_agent_position(a);
    float3 b_p = float3_get_agent_position(b);

    float dx_sq = b_p.x - a_p.x;
    float dy_sq = b_p.y - a_p.y;
    float dz_sq = b_p.z - a_p.z;
    dx_sq *= dx_sq;
    dy_sq *= dy_sq;
    dz_sq *= dz_sq;
    float d_sq = dx_sq + dy_sq + dz_sq;

    /* separation */

    /* cohesion */

    /* alignment */
}

void agent_act(Agent *a, ActContext *c) {
    float3 p = float3_get_agent_position(a);
    float3_set_agent_position(a, float3_add(p, a->v));
    a->separation.x = 0.0f;
    a->separation.y = 0.0f;
    a->separation.z = 0.0f;
    a->cohesion.x = 0.0f;
    a->cohesion.y = 0.0f;
    a->cohesion.z = 0.0f;
    a->alignment.x = 0.0f;
    a->alignment.y = 0.0f;
    a->alignment.z = 0.0f;
}


float2 float2_get_agent_position(void *agent) {
    return *(float2 *)((TayAgentTag *)agent + 1);
}

float3 float3_get_agent_position(void *agent) {
    return *(float3 *)((TayAgentTag *)agent + 1);
}

float4 float4_get_agent_position(void *agent) {
    return *(float4 *)((TayAgentTag *)agent + 1);
}

void float2_set_agent_position(void *agent, float2 p) {
    *(float2 *)((TayAgentTag *)agent + 1) = p;
}

void float3_set_agent_position(void *agent, float3 p) {
    *(float3 *)((TayAgentTag *)agent + 1) = p;
}

void float4_set_agent_position(void *agent, float4 p) {
    *(float4 *)((TayAgentTag *)agent + 1) = p;
}

float3 float3_null() {
    float3 r;
    r.x = 0.0f;
    r.y = 0.0f;
    r.z = 0.0f;
    return r;
}

float3 float3_make(float x, float y, float z) {
    float3 r;
    r.x = x;
    r.y = y;
    r.z = z;
    return r;
}

float3 float3_add(float3 a, float3 b) {
    float3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

float3 float3_sub(float3 a, float3 b) {
    float3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

float3 float3_div_scalar(float3 a, float s) {
    float3 r;
    r.x = a.x / s;
    r.y = a.y / s;
    r.z = a.z / s;
    return r;
}

const char *agent_kernels_source = "\n\
typedef struct __attribute__((packed)) Agent {\n\
    TayAgentTag tag;\n\
    float4 p;\n\
    float3 v;\n\
    float3 separation;\n\
    float3 cohesion;\n\
    float3 alignment;\n\
} Agent;\n\
\n\
typedef struct __attribute__((packed)) ActContext {\n\
    int dummy;\n\
} ActContext;\n\
\n\
typedef struct __attribute__((packed)) SeeContext {\n\
    float4 radii_sq;\n\
} SeeContext;\n\
\n\
\n\
float2 float2_get_agent_position(global void *agent) {\n\
    return *(global float2 *)((global TayAgentTag *)agent + 1);\n\
}\n\
\n\
float3 float3_get_agent_position(global void *agent) {\n\
    return *(global float3 *)((global TayAgentTag *)agent + 1);\n\
}\n\
\n\
float4 float4_get_agent_position(global void *agent) {\n\
    return *(global float4 *)((global TayAgentTag *)agent + 1);\n\
}\n\
\n\
void float2_set_agent_position(global void *agent, float2 p) {\n\
    *(global float2 *)((global TayAgentTag *)agent + 1) = p;\n\
}\n\
\n\
void float3_set_agent_position(global void *agent, float3 p) {\n\
    *(global float3 *)((global TayAgentTag *)agent + 1) = p;\n\
}\n\
\n\
void float4_set_agent_position(global void *agent, float4 p) {\n\
    *(global float4 *)((global TayAgentTag *)agent + 1) = p;\n\
}\n\
\n\
float3 float3_null() {\n\
    float3 r;\n\
    r.x = 0.0f;\n\
    r.y = 0.0f;\n\
    r.z = 0.0f;\n\
    return r;\n\
}\n\
\n\
float3 float3_make(float x, float y, float z) {\n\
    float3 r;\n\
    r.x = x;\n\
    r.y = y;\n\
    r.z = z;\n\
    return r;\n\
}\n\
\n\
float3 float3_add(float3 a, float3 b) {\n\
    float3 r;\n\
    r.x = a.x + b.x;\n\
    r.y = a.y + b.y;\n\
    r.z = a.z + b.z;\n\
    return r;\n\
}\n\
\n\
float3 float3_sub(float3 a, float3 b) {\n\
    float3 r;\n\
    r.x = a.x - b.x;\n\
    r.y = a.y - b.y;\n\
    r.z = a.z - b.z;\n\
    return r;\n\
}\n\
\n\
float3 float3_div_scalar(float3 a, float s) {\n\
    float3 r;\n\
    r.x = a.x / s;\n\
    r.y = a.y / s;\n\
    r.z = a.z / s;\n\
    return r;\n\
}\n\
\n\
void agent_see(global Agent *a, global Agent *b, global SeeContext *c) {\n\
    float3 a_p = float3_get_agent_position(a);\n\
    float3 b_p = float3_get_agent_position(b);\n\
\n\
    float dx_sq = b_p.x - a_p.x;\n\
    float dy_sq = b_p.y - a_p.y;\n\
    float dz_sq = b_p.z - a_p.z;\n\
    dx_sq *= dx_sq;\n\
    dy_sq *= dy_sq;\n\
    dz_sq *= dz_sq;\n\
    float d_sq = dx_sq + dy_sq + dz_sq;\n\
\n\
    /* separation */\n\
\n\
    /* cohesion */\n\
\n\
    /* alignment */\n\
}\n\
\n\
void agent_act(global Agent *a, global ActContext *c) {\n\
    float3 p = float3_get_agent_position(a);\n\
    float3_set_agent_position(a, float3_add(p, a->v));\n\
    a->separation.x = 0.0f;\n\
    a->separation.y = 0.0f;\n\
    a->separation.z = 0.0f;\n\
    a->cohesion.x = 0.0f;\n\
    a->cohesion.y = 0.0f;\n\
    a->cohesion.z = 0.0f;\n\
    a->alignment.x = 0.0f;\n\
    a->alignment.y = 0.0f;\n\
    a->alignment.z = 0.0f;\n\
}\n\
\n\
";
