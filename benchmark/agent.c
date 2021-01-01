#include "agent.h"


void agent_see(Agent *a, Agent *b, SeeContext *c) {
    float3 a_p = float3_get_agent_position(a);
    float3 b_p = float3_get_agent_position(b);
    for (int i = 0; i < 1; ++i)
        a->b_buffer = float3_add(a->b_buffer, float3_sub(b_p, a_p));
    a->b_buffer_count++;
}

void agent_act(Agent *agent, ActContext *c) {

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        float3 n = float3_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->f_buffer = float3_add(agent->f_buffer, n);
        agent->b_buffer = float3_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    float3 p = float3_get_agent_position(agent);
    float3_set_agent_position(agent, float3_add(p, agent->v));

    if (agent->p.x < c->min.x) {
        agent->p.x = c->min.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->p.y < c->min.y) {
        agent->p.y = c->min.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->p.z < c->min.z) {
        agent->p.z = c->min.z;
        agent->v.z = -agent->v.z;
    }

    if (agent->p.x > c->max.x) {
        agent->p.x = c->max.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->p.y > c->max.y) {
        agent->p.y = c->max.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->p.z > c->max.z) {
        agent->p.z = c->max.z;
        agent->v.z = -agent->v.z;
    }
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
    float3 b_buffer;\n\
    int b_buffer_count;\n\
    float3 f_buffer;\n\
} Agent;\n\
\n\
typedef struct __attribute__((packed)) ActContext {\n\
    float4 min;\n\
    float4 max;\n\
} ActContext;\n\
\n\
typedef struct __attribute__((packed)) SeeContext {\n\
    float4 radii;\n\
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
    for (int i = 0; i < 1; ++i)\n\
        a->b_buffer = float3_add(a->b_buffer, float3_sub(b_p, a_p));\n\
    a->b_buffer_count++;\n\
}\n\
\n\
void agent_act(global Agent *agent, global ActContext *c) {\n\
\n\
    /* buffer swap */\n\
\n\
    if (agent->b_buffer_count != 0) {\n\
        float3 n = float3_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);\n\
        agent->f_buffer = float3_add(agent->f_buffer, n);\n\
        agent->b_buffer = float3_null();\n\
        agent->b_buffer_count = 0;\n\
    }\n\
\n\
    /* move */\n\
\n\
    float3 p = float3_get_agent_position(agent);\n\
    float3_set_agent_position(agent, float3_add(p, agent->v));\n\
\n\
    if (agent->p.x < c->min.x) {\n\
        agent->p.x = c->min.x;\n\
        agent->v.x = -agent->v.x;\n\
    }\n\
\n\
    if (agent->p.y < c->min.y) {\n\
        agent->p.y = c->min.y;\n\
        agent->v.y = -agent->v.y;\n\
    }\n\
\n\
    if (agent->p.z < c->min.z) {\n\
        agent->p.z = c->min.z;\n\
        agent->v.z = -agent->v.z;\n\
    }\n\
\n\
    if (agent->p.x > c->max.x) {\n\
        agent->p.x = c->max.x;\n\
        agent->v.x = -agent->v.x;\n\
    }\n\
\n\
    if (agent->p.y > c->max.y) {\n\
        agent->p.y = c->max.y;\n\
        agent->v.y = -agent->v.y;\n\
    }\n\
\n\
    if (agent->p.z > c->max.z) {\n\
        agent->p.z = c->max.z;\n\
        agent->v.z = -agent->v.z;\n\
    }\n\
}\n\
\n\
";
