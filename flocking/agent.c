#include "agent.h"
#include "tay.h"
#include <math.h>


void agent_see(Agent *a, Agent *b, SeeContext *c) {
    float3 a_p = float3_agent_position(a);
    float3 b_p = float3_agent_position(b);

    float dx = b_p.x - a_p.x;
    float dy = b_p.y - a_p.y;
    float dz = b_p.z - a_p.z;
    float d_sq = dx * dx + dy * dy + dz * dz;

    if (d_sq >= c->r_sq) /* narrow narrow phase */
        return;

    float d = (float)sqrt(d_sq);

    float f = 0.0f;
    if (d < c->r1)
        f = c->repulsion;
    else if (d < c->r2)
        f = c->repulsion + (c->attraction - c->repulsion) * (d - c->r1) / (c->r2 - c->r1);
    else
        f = c->attraction + c->attraction * (1.0f - (d - c->r2) / (c->r - c->r2));

    a->f.x += f * dx / d;
    a->f.y += f * dy / d;
    a->f.z += f * dz / d;

    ++a->seen;
}

void agent_act(Agent *a, ActContext *c) {
    float3 p = float3_agent_position(a);

    /* calculate force */

    const float gravity = 0.000001f;

    float3 f;
    if (a->seen) {
        f = float3_mul_scalar(float3_div_scalar(a->f, (float)a->seen), 0.2f);
        f.x -= p.x * gravity;
        f.y -= p.y * gravity;
        f.z -= p.z * gravity;
    }
    else {
        f.x = -p.x * gravity;
        f.y = -p.y * gravity;
        f.z = -p.z * gravity;
    }

    /* velocity from force */

    const float min_v = 0.2f;
    const float max_v = 0.3f;

    a->v = float3_add(a->v, f);
    float v = float3_length(a->v);

    if (v < 0.000001f) {
        a->v.x = min_v;
        a->v.y = 0.0f;
        a->v.z = 0.0f;
    }
    else if (v < min_v) {
        a->v.x *= min_v / v;
        a->v.y *= min_v / v;
        a->v.z *= min_v / v;
    }
    else if (v > max_v) {
        a->v.x *= max_v / v;
        a->v.y *= max_v / v;
        a->v.z *= max_v / v;
    }

    float3_agent_position(a) = float3_add(p, a->v);

    a->f = float3_null();
    a->seen = 0;
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
        r.x = b;
        r.y = 0.0f;
        r.z = 0.0f;
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
    float3 f;\n\
    int seen;\n\
} Agent;\n\
\n\
typedef struct __attribute__((packed)) ActContext {\n\
    int dummy;\n\
} ActContext;\n\
\n\
typedef struct __attribute__((packed)) SeeContext {\n\
    float r_sq;\n\
    float r;\n\
    float r1;\n\
    float r2;\n\
    float repulsion;\n\
    float attraction;\n\
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
        r.x = b;\n\
        r.y = 0.0f;\n\
        r.z = 0.0f;\n\
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
    float dx = b_p.x - a_p.x;\n\
    float dy = b_p.y - a_p.y;\n\
    float dz = b_p.z - a_p.z;\n\
    float d_sq = dx * dx + dy * dy + dz * dz;\n\
\n\
    if (d_sq >= c->r_sq) /* narrow narrow phase */\n\
        return;\n\
\n\
    float d = (float)sqrt(d_sq);\n\
\n\
    float f = 0.0f;\n\
    if (d < c->r1)\n\
        f = c->repulsion;\n\
    else if (d < c->r2)\n\
        f = c->repulsion + (c->attraction - c->repulsion) * (d - c->r1) / (c->r2 - c->r1);\n\
    else\n\
        f = c->attraction + c->attraction * (1.0f - (d - c->r2) / (c->r - c->r2));\n\
\n\
    a->f.x += f * dx / d;\n\
    a->f.y += f * dy / d;\n\
    a->f.z += f * dz / d;\n\
\n\
    ++a->seen;\n\
}\n\
\n\
void agent_act(global Agent *a, global ActContext *c) {\n\
    float3 p = float3_agent_position(a);\n\
\n\
    /* calculate force */\n\
\n\
    const float gravity = 0.000001f;\n\
\n\
    float3 f;\n\
    if (a->seen) {\n\
        f = float3_mul_scalar(float3_div_scalar(a->f, (float)a->seen), 0.2f);\n\
        f.x -= p.x * gravity;\n\
        f.y -= p.y * gravity;\n\
        f.z -= p.z * gravity;\n\
    }\n\
    else {\n\
        f.x = -p.x * gravity;\n\
        f.y = -p.y * gravity;\n\
        f.z = -p.z * gravity;\n\
    }\n\
\n\
    /* velocity from force */\n\
\n\
    const float min_v = 0.2f;\n\
    const float max_v = 0.3f;\n\
\n\
    a->v = float3_add(a->v, f);\n\
    float v = float3_length(a->v);\n\
\n\
    if (v < 0.000001f) {\n\
        a->v.x = min_v;\n\
        a->v.y = 0.0f;\n\
        a->v.z = 0.0f;\n\
    }\n\
    else if (v < min_v) {\n\
        a->v.x *= min_v / v;\n\
        a->v.y *= min_v / v;\n\
        a->v.z *= min_v / v;\n\
    }\n\
    else if (v > max_v) {\n\
        a->v.x *= max_v / v;\n\
        a->v.y *= max_v / v;\n\
        a->v.z *= max_v / v;\n\
    }\n\
\n\
    float3_agent_position(a) = float3_add(p, a->v);\n\
\n\
    a->f = float3_null();\n\
    a->seen = 0;\n\
}\n\
\n\
";
