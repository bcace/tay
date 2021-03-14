#include "agent.h"
#include "tay.h"
#include <math.h>


void agent_see(Agent *a, Agent *b, SeeContext *c) {
    float4 a_p = float4_agent_position(a);
    float4 b_p = float4_agent_position(b);
    for (int i = 0; i < 1; ++i)
        a->b_buffer = float4_add(a->b_buffer, float4_sub(b_p, a_p));
    a->b_buffer_count++;
}

void agent_act(Agent *agent, ActContext *c) {

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        float4 n = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->f_buffer = float4_add(agent->f_buffer, n);
        agent->b_buffer = float4_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    float4 p = float4_agent_position(agent);
    float4_agent_position(agent) = float4_add(p, agent->v);

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

void box_agent_see(BoxAgent *a, BoxAgent *b, SeeContext *c) {
    float4 a_min = float4_agent_min(a);
    float4 b_min = float4_agent_min(b);
    for (int i = 0; i < 1; ++i)
        a->b_buffer = float4_add(a->b_buffer, float4_sub(b_min, a_min));
    a->b_buffer_count++;
}

void box_agent_act(BoxAgent *agent, ActContext *c) {

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        float4 n = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->f_buffer = float4_add(agent->f_buffer, n);
        agent->b_buffer = float4_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    float4 min = float4_agent_min(agent);
    float4 max = float4_agent_max(agent);
    float4 size = float4_sub(max, min);
    float4_agent_min(agent) = float4_add(min, agent->v);
    float4_agent_max(agent) = float4_add(max, agent->v);

    if (agent->min.x < c->min.x) {
        agent->min.x = c->min.x;
        agent->max.x = c->min.x + size.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->min.y < c->min.y) {
        agent->min.y = c->min.y;
        agent->max.y = c->min.y + size.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->min.z < c->min.z) {
        agent->min.z = c->min.z;
        agent->max.z = c->min.z + size.z;
        agent->v.z = -agent->v.z;
    }

    if (agent->max.x > c->max.x) {
        agent->max.x = c->max.x;
        agent->min.x = c->max.x - size.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->max.y > c->max.y) {
        agent->max.y = c->max.y;
        agent->min.y = c->max.y - size.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->max.z > c->max.z) {
        agent->max.z = c->max.z;
        agent->min.z = c->max.z - size.z;
        agent->v.z = -agent->v.z;
    }
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

float3 float3_mul_scalar(float3 a, float s) {
    float3 r;
    r.x = a.x * s;
    r.y = a.y * s;
    r.z = a.z * s;
    return r;
}

float3 float3_normalize(float3 a) {
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    float3 r;
    if (l < 0.000001f) {
        r.x = 1.0f;
        r.y = 0.0f;
        r.z = 0.0f;
    }
    else {
        r.x = a.x / l;
        r.y = a.y / l;
        r.z = a.z / l;
    }
    return r;
}

float3 float3_normalize_to(float3 a, float b) {
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    float3 r;
    if (l < 0.000001f) {
        r.x = a.x;
        r.y = a.y;
        r.z = a.z;
    }
    else {
        float c = b / l;
        r.x = a.x * c;
        r.y = a.y * c;
        r.z = a.z * c;
    }
    return r;
}

float float3_length(float3 a) {
    return (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

float float3_dot(float3 a, float3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
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
typedef struct __attribute__((packed)) BoxAgent {\n\
    TayAgentTag tag;\n\
    float4 min;\n\
    float4 max;\n\
    float4 v;\n\
    float4 b_buffer;\n\
    int b_buffer_count;\n\
    float4 f_buffer;\n\
} BoxAgent;\n\
\n\
void agent_see(global Agent *a, global Agent *b, global SeeContext *context);\n\
void agent_act(global Agent *agent, global ActContext *context);\n\
\n\
void box_agent_see(global BoxAgent *a, global BoxAgent *b, global SeeContext *context);\n\
void box_agent_act(global BoxAgent *agent, global ActContext *context);\n\
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
float3 float3_mul_scalar(float3 a, float s) {\n\
    float3 r;\n\
    r.x = a.x * s;\n\
    r.y = a.y * s;\n\
    r.z = a.z * s;\n\
    return r;\n\
}\n\
\n\
float3 float3_normalize(float3 a) {\n\
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n\
    float3 r;\n\
    if (l < 0.000001f) {\n\
        r.x = 1.0f;\n\
        r.y = 0.0f;\n\
        r.z = 0.0f;\n\
    }\n\
    else {\n\
        r.x = a.x / l;\n\
        r.y = a.y / l;\n\
        r.z = a.z / l;\n\
    }\n\
    return r;\n\
}\n\
\n\
float3 float3_normalize_to(float3 a, float b) {\n\
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n\
    float3 r;\n\
    if (l < 0.000001f) {\n\
        r.x = a.x;\n\
        r.y = a.y;\n\
        r.z = a.z;\n\
    }\n\
    else {\n\
        float c = b / l;\n\
        r.x = a.x * c;\n\
        r.y = a.y * c;\n\
        r.z = a.z * c;\n\
    }\n\
    return r;\n\
}\n\
\n\
float float3_length(float3 a) {\n\
    return (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n\
}\n\
\n\
float float3_dot(float3 a, float3 b) {\n\
    return a.x * b.x + a.y * b.y + a.z * b.z;\n\
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
    float4 a_p = float4_agent_position(a);\n\
    float4 b_p = float4_agent_position(b);\n\
    for (int i = 0; i < 1; ++i)\n\
        a->b_buffer = float4_add(a->b_buffer, float4_sub(b_p, a_p));\n\
    a->b_buffer_count++;\n\
}\n\
\n\
void agent_act(global Agent *agent, global ActContext *c) {\n\
\n\
    /* buffer swap */\n\
\n\
    if (agent->b_buffer_count != 0) {\n\
        float4 n = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);\n\
        agent->f_buffer = float4_add(agent->f_buffer, n);\n\
        agent->b_buffer = float4_null();\n\
        agent->b_buffer_count = 0;\n\
    }\n\
\n\
    /* move */\n\
\n\
    float4 p = float4_agent_position(agent);\n\
    float4_agent_position(agent) = float4_add(p, agent->v);\n\
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
void box_agent_see(global BoxAgent *a, global BoxAgent *b, global SeeContext *c) {\n\
    float4 a_min = float4_agent_min(a);\n\
    float4 b_min = float4_agent_min(b);\n\
    for (int i = 0; i < 1; ++i)\n\
        a->b_buffer = float4_add(a->b_buffer, float4_sub(b_min, a_min));\n\
    a->b_buffer_count++;\n\
}\n\
\n\
void box_agent_act(global BoxAgent *agent, global ActContext *c) {\n\
\n\
    /* buffer swap */\n\
\n\
    if (agent->b_buffer_count != 0) {\n\
        float4 n = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);\n\
        agent->f_buffer = float4_add(agent->f_buffer, n);\n\
        agent->b_buffer = float4_null();\n\
        agent->b_buffer_count = 0;\n\
    }\n\
\n\
    /* move */\n\
\n\
    float4 min = float4_agent_min(agent);\n\
    float4 max = float4_agent_max(agent);\n\
    float4 size = float4_sub(max, min);\n\
    float4_agent_min(agent) = float4_add(min, agent->v);\n\
    float4_agent_max(agent) = float4_add(max, agent->v);\n\
\n\
    if (agent->min.x < c->min.x) {\n\
        agent->min.x = c->min.x;\n\
        agent->max.x = c->min.x + size.x;\n\
        agent->v.x = -agent->v.x;\n\
    }\n\
\n\
    if (agent->min.y < c->min.y) {\n\
        agent->min.y = c->min.y;\n\
        agent->max.y = c->min.y + size.y;\n\
        agent->v.y = -agent->v.y;\n\
    }\n\
\n\
    if (agent->min.z < c->min.z) {\n\
        agent->min.z = c->min.z;\n\
        agent->max.z = c->min.z + size.z;\n\
        agent->v.z = -agent->v.z;\n\
    }\n\
\n\
    if (agent->max.x > c->max.x) {\n\
        agent->max.x = c->max.x;\n\
        agent->min.x = c->max.x - size.x;\n\
        agent->v.x = -agent->v.x;\n\
    }\n\
\n\
    if (agent->max.y > c->max.y) {\n\
        agent->max.y = c->max.y;\n\
        agent->min.y = c->max.y - size.y;\n\
        agent->v.y = -agent->v.y;\n\
    }\n\
\n\
    if (agent->max.z > c->max.z) {\n\
        agent->max.z = c->max.z;\n\
        agent->min.z = c->max.z - size.z;\n\
        agent->v.z = -agent->v.z;\n\
    }\n\
}\n\
\n\
";
