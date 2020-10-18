#include "agent.h"


void agent_see(Agent *a, Agent *b, SeeContext *c) {
    a->b_buffer = float4_add(a->b_buffer, float4_sub(b->p, a->p));
    a->b_buffer_count++;
}

void agent_act(Agent *agent, ActContext *c) {

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        agent->f_buffer = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->b_buffer = float4_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    agent->p = float4_add(agent->p, agent->v);

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

float4 float4_null() {
    float4 r;
    r.x = r.y = r.z = 0.0f;
    return r;
}

float4 float4_make(float x, float y, float z) {
    float4 r;
    r.x = x;
    r.y = y;
    r.z = z;
    return r;
}

float4 float4_add(float4 a, float4 b) {
    float4 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

float4 float4_sub(float4 a, float4 b) {
    float4 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

float4 float4_div_scalar(float4 a, float s) {
    float4 r;
    r.x = a.x / s;
    r.y = a.y / s;
    r.z = a.z / s;
    return r;
}

const char *agent_kernels_source = "\n\
\n\
typedef struct __attribute__((packed)) TayAgentTag {\n\
    int next;\n\
    int padding;\n\
} TayAgentTag;\n\
\n\
\n\
typedef struct __attribute__((packed)) Agent {\n\
    TayAgentTag tag;\n\
    float4 p;\n\
    float4 v;\n\
    float4 b_buffer;\n\
    int b_buffer_count;\n\
    float4 f_buffer;\n\
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
float4 float4_null() {\n\
    float4 r;\n\
    r.x = r.y = r.z = 0.0f;\n\
    return r;\n\
}\n\
\n\
float4 float4_make(float x, float y, float z) {\n\
    float4 r;\n\
    r.x = x;\n\
    r.y = y;\n\
    r.z = z;\n\
    return r;\n\
}\n\
\n\
float4 float4_add(float4 a, float4 b) {\n\
    float4 r;\n\
    r.x = a.x + b.x;\n\
    r.y = a.y + b.y;\n\
    r.z = a.z + b.z;\n\
    return r;\n\
}\n\
\n\
float4 float4_sub(float4 a, float4 b) {\n\
    float4 r;\n\
    r.x = a.x - b.x;\n\
    r.y = a.y - b.y;\n\
    r.z = a.z - b.z;\n\
    return r;\n\
}\n\
\n\
float4 float4_div_scalar(float4 a, float s) {\n\
    float4 r;\n\
    r.x = a.x / s;\n\
    r.y = a.y / s;\n\
    r.z = a.z / s;\n\
    return r;\n\
}\n\
\n\
void agent_see(global Agent *a, global Agent *b, global SeeContext *c) {\n\
    a->b_buffer = float4_add(a->b_buffer, float4_sub(b->p, a->p));\n\
    a->b_buffer_count++;\n\
}\n\
\n\
void agent_act(global Agent *agent, global ActContext *c) {\n\
\n\
    /* buffer swap */\n\
\n\
    if (agent->b_buffer_count != 0) {\n\
        agent->f_buffer = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);\n\
        agent->b_buffer = float4_null();\n\
        agent->b_buffer_count = 0;\n\
    }\n\
\n\
    /* move */\n\
\n\
    agent->p = float4_add(agent->p, agent->v);\n\
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
