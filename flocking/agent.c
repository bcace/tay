#include "agent.h"
#include "tay.h"


void agent_see(Agent *a, Agent *b, SeeContext *c) {
    float3 a_p = float3_agent_position(a);
    float3 b_p = float3_agent_position(b);

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
    float3 p = float3_agent_position(a);
    float3_agent_position(a) = float3_add(p, a->v);
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

float4 float4_null() {
    float4 r;
    r.x = 0.0f;
    r.y = 0.0f;
    r.z = 0.0f;
    r.w = 0.0f;
    return r;
}

float4 float4_make(float x, float y, float z, float w) {
    float4 r;
    r.x = x;
    r.y = y;
    r.z = z;
    r.w = w;
    return r;
}

float4 float4_add(float4 a, float4 b) {
    float4 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    r.w = a.w + b.w;
    return r;
}

float4 float4_sub(float4 a, float4 b) {
    float4 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    r.w = a.w - b.w;
    return r;
}

float4 float4_div_scalar(float4 a, float s) {
    float4 r;
    r.x = a.x / s;
    r.y = a.y / s;
    r.z = a.z / s;
    r.w = a.w / s;
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
float4 float4_null() {\n\
    float4 r;\n\
    r.x = 0.0f;\n\
    r.y = 0.0f;\n\
    r.z = 0.0f;\n\
    r.w = 0.0f;\n\
    return r;\n\
}\n\
\n\
float4 float4_make(float x, float y, float z, float w) {\n\
    float4 r;\n\
    r.x = x;\n\
    r.y = y;\n\
    r.z = z;\n\
    r.w = w;\n\
    return r;\n\
}\n\
\n\
float4 float4_add(float4 a, float4 b) {\n\
    float4 r;\n\
    r.x = a.x + b.x;\n\
    r.y = a.y + b.y;\n\
    r.z = a.z + b.z;\n\
    r.w = a.w + b.w;\n\
    return r;\n\
}\n\
\n\
float4 float4_sub(float4 a, float4 b) {\n\
    float4 r;\n\
    r.x = a.x - b.x;\n\
    r.y = a.y - b.y;\n\
    r.z = a.z - b.z;\n\
    r.w = a.w - b.w;\n\
    return r;\n\
}\n\
\n\
float4 float4_div_scalar(float4 a, float s) {\n\
    float4 r;\n\
    r.x = a.x / s;\n\
    r.y = a.y / s;\n\
    r.z = a.z / s;\n\
    r.w = a.w / s;\n\
    return r;\n\
}\n\
\n\
void agent_see(global Agent *a, global Agent *b, global SeeContext *c) {\n\
    float3 a_p = float3_agent_position(a);\n\
    float3 b_p = float3_agent_position(b);\n\
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
    float3 p = float3_agent_position(a);\n\
    float3_agent_position(a) = float3_add(p, a->v);\n\
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
