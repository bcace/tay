#include "agent.h"
#include "tay.h"
#include <math.h>


void agent_see(Agent *a, Agent *b, SeeContext *c) {
    float3 a_p = float3_agent_position(a);
    float3 b_p = float3_agent_position(b);

    float3 d = float3_sub(b_p, a_p);
    float dl = float3_length(d);

    if (dl >= c->r) /* narrow narrow phase */
        return;

    /* alignment */

    a->alignment = float3_add(a->alignment, b->dir);

    float dot = float3_dot(a->dir, d);
    if (dot > 0.0f) {

        /* separation */

        const float separation_f = 0.5f;

        if (dl < (c->r * separation_f)) {
            float dot = float3_dot(a->dir, d);
            if (dot > 0.0f)
                a->separation = float3_sub(a->separation, float3_sub(d, float3_mul_scalar(a->dir, -dot)));
        }

        /* cohesion */

        a->cohesion = float3_add(a->cohesion, d);
        ++a->cohesion_count;
    }
}

void agent_act(Agent *a, ActContext *c) {
    float3 p = float3_agent_position(a);
    float3 acc = float3_null();

    /* gravity */

    const float gravity_r = 200.0f;
    const float gravity_a = 0.00001f;

    float d = (float)sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    if (d > gravity_r)
        acc = float3_sub(acc, float3_mul_scalar(p, (d - gravity_r) * gravity_a / d));

    /* alignment */

    const float alignment_a = 0.01f;

    acc = float3_add(acc, float3_normalize_to(a->alignment, alignment_a));

    /* separation */

    const float separation_a = 0.03f;

    acc = float3_add(acc, float3_normalize_to(a->separation, separation_a));

    /* cohesion */

    const float cohesion_a = 0.01f;

    acc = float3_add(acc, float3_normalize_to(a->cohesion, cohesion_a));

    /* update */

    a->dir = float3_normalize(float3_add(a->dir, acc));
    float3_agent_position(a) = float3_add(p, float3_mul_scalar(a->dir, a->speed));

    a->separation = float3_null();
    a->alignment = float3_null();
    a->cohesion = float3_null();
    a->cohesion_count = 0;
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
    float3 dir;\n\
    float speed;\n\
    float3 separation;\n\
    float3 alignment;\n\
    float3 cohesion;\n\
    int cohesion_count;\n\
} Agent;\n\
\n\
typedef struct __attribute__((packed)) ActContext {\n\
    int dummy;\n\
} ActContext;\n\
\n\
typedef struct __attribute__((packed)) SeeContext {\n\
    float r_sq;\n\
    float r;\n\
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
    float3 a_p = float3_agent_position(a);\n\
    float3 b_p = float3_agent_position(b);\n\
\n\
    float3 d = float3_sub(b_p, a_p);\n\
    float dl = float3_length(d);\n\
\n\
    if (dl >= c->r) /* narrow narrow phase */\n\
        return;\n\
\n\
    /* alignment */\n\
\n\
    a->alignment = float3_add(a->alignment, b->dir);\n\
\n\
    float dot = float3_dot(a->dir, d);\n\
    if (dot > 0.0f) {\n\
\n\
        /* separation */\n\
\n\
        const float separation_f = 0.5f;\n\
\n\
        if (dl < (c->r * separation_f)) {\n\
            float dot = float3_dot(a->dir, d);\n\
            if (dot > 0.0f)\n\
                a->separation = float3_sub(a->separation, float3_sub(d, float3_mul_scalar(a->dir, -dot)));\n\
        }\n\
\n\
        /* cohesion */\n\
\n\
        a->cohesion = float3_add(a->cohesion, d);\n\
        ++a->cohesion_count;\n\
    }\n\
}\n\
\n\
void agent_act(global Agent *a, global ActContext *c) {\n\
    float3 p = float3_agent_position(a);\n\
    float3 acc = float3_null();\n\
\n\
    /* gravity */\n\
\n\
    const float gravity_r = 200.0f;\n\
    const float gravity_a = 0.00001f;\n\
\n\
    float d = (float)sqrt(p.x * p.x + p.y * p.y + p.z * p.z);\n\
    if (d > gravity_r)\n\
        acc = float3_sub(acc, float3_mul_scalar(p, (d - gravity_r) * gravity_a / d));\n\
\n\
    /* alignment */\n\
\n\
    const float alignment_a = 0.01f;\n\
\n\
    acc = float3_add(acc, float3_normalize_to(a->alignment, alignment_a));\n\
\n\
    /* separation */\n\
\n\
    const float separation_a = 0.03f;\n\
\n\
    acc = float3_add(acc, float3_normalize_to(a->separation, separation_a));\n\
\n\
    /* cohesion */\n\
\n\
    const float cohesion_a = 0.01f;\n\
\n\
    acc = float3_add(acc, float3_normalize_to(a->cohesion, cohesion_a));\n\
\n\
    /* update */\n\
\n\
    a->dir = float3_normalize(float3_add(a->dir, acc));\n\
    float3_agent_position(a) = float3_add(p, float3_mul_scalar(a->dir, a->speed));\n\
\n\
    a->separation = float3_null();\n\
    a->alignment = float3_null();\n\
    a->cohesion = float3_null();\n\
    a->cohesion_count = 0;\n\
}\n\
\n\
";
