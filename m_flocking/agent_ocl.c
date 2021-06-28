const char *agent_ocl_c = "\n \
void agent_see(global Agent *a, global Agent *b, constant SeeContext *c) {\n \
    float4 a_p = float4_agent_position(a);\n \
    float4 b_p = float4_agent_position(b);\n \
\n \
    float4 d = float4_sub(b_p, a_p);\n \
    float dl = float4_length(d);\n \
\n \
    if (dl >= c->r) /* narrow narrow phase */\n \
        return;\n \
\n \
    /* forces are only calculated for neighbors in front of the boid */\n \
\n \
    float dot = float4_dot(a->dir, d);\n \
    if (dot > 0.0f) {\n \
\n \
        /* alignment */\n \
\n \
        a->alignment = float4_add(a->alignment, b->dir);\n \
\n \
        /* separation */\n \
\n \
        if (dl < c->separation_r) {\n \
            a->separation = float4_sub(a->separation, float4_normalize_to(d, c->separation_r - dl));\n \
            ++a->separation_count;\n \
        }\n \
\n \
        /* cohesion */\n \
\n \
        a->cohesion = float4_add(a->cohesion, d);\n \
        ++a->cohesion_count;\n \
    }\n \
}\n \
\n \
void agent_act(global Agent *a, constant ActContext *c) {\n \
    float4 p = float4_agent_position(a);\n \
    float4 acc = float4_null();\n \
\n \
    /* gravity */\n \
\n \
    const float gravity_r = 400.0f;\n \
    const float gravity_a = 0.00001f;\n \
\n \
    float d = (float)sqrt(p.x * p.x + p.y * p.y + p.z * p.z);\n \
    if (d > gravity_r)\n \
        acc = float4_sub(acc, float4_mul_scalar(p, (d - gravity_r) * gravity_a / d));\n \
\n \
    /* alignment */\n \
\n \
    const float alignment_a = 0.02f;\n \
\n \
    if (a->cohesion_count)\n \
        acc = float4_add(acc, float4_mul_scalar(a->alignment, alignment_a / (float)a->cohesion_count));\n \
\n \
    /* separation */\n \
\n \
    const float separation_a = 0.02f;\n \
\n \
    if (a->separation_count)\n \
        acc = float4_add(acc, float4_mul_scalar(a->separation, separation_a / (float)a->separation_count));\n \
\n \
    /* cohesion */\n \
\n \
    const float cohesion_a = 0.001f;\n \
\n \
    if (a->cohesion_count)\n \
        acc = float4_add(acc, float4_mul_scalar(a->cohesion, cohesion_a / (float)a->cohesion_count));\n \
\n \
    /* update */\n \
\n \
    a->dir = float4_normalize(float4_add(a->dir, acc));\n \
    float4_agent_position(a) = float4_add(p, float4_mul_scalar(a->dir, a->speed));\n \
\n \
    a->separation = float4_null();\n \
    a->alignment = float4_null();\n \
    a->cohesion = float4_null();\n \
    a->cohesion_count = 0;\n \
    a->separation_count = 0;\n \
}\n \
";

const char *taystd_ocl_c = "\n \
float3 float3_null() {\n \
    float3 r;\n \
    r.x = 0.0f;\n \
    r.y = 0.0f;\n \
    r.z = 0.0f;\n \
    return r;\n \
}\n \
\n \
float3 float3_make(float x, float y, float z) {\n \
    float3 r;\n \
    r.x = x;\n \
    r.y = y;\n \
    r.z = z;\n \
    return r;\n \
}\n \
\n \
float3 float3_add(float3 a, float3 b) {\n \
    float3 r;\n \
    r.x = a.x + b.x;\n \
    r.y = a.y + b.y;\n \
    r.z = a.z + b.z;\n \
    return r;\n \
}\n \
\n \
float3 float3_sub(float3 a, float3 b) {\n \
    float3 r;\n \
    r.x = a.x - b.x;\n \
    r.y = a.y - b.y;\n \
    r.z = a.z - b.z;\n \
    return r;\n \
}\n \
\n \
float3 float3_div_scalar(float3 a, float s) {\n \
    float3 r;\n \
    r.x = a.x / s;\n \
    r.y = a.y / s;\n \
    r.z = a.z / s;\n \
    return r;\n \
}\n \
\n \
float3 float3_mul_scalar(float3 a, float s) {\n \
    float3 r;\n \
    r.x = a.x * s;\n \
    r.y = a.y * s;\n \
    r.z = a.z * s;\n \
    return r;\n \
}\n \
\n \
float3 float3_normalize(float3 a) {\n \
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n \
    float3 r;\n \
    if (l < 0.000001f) {\n \
        r.x = 1.0f;\n \
        r.y = 0.0f;\n \
        r.z = 0.0f;\n \
    }\n \
    else {\n \
        r.x = a.x / l;\n \
        r.y = a.y / l;\n \
        r.z = a.z / l;\n \
    }\n \
    return r;\n \
}\n \
\n \
float3 float3_normalize_to(float3 a, float b) {\n \
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n \
    float3 r;\n \
    if (l < 0.000001f) {\n \
        r.x = a.x;\n \
        r.y = a.y;\n \
        r.z = a.z;\n \
    }\n \
    else {\n \
        float c = b / l;\n \
        r.x = a.x * c;\n \
        r.y = a.y * c;\n \
        r.z = a.z * c;\n \
    }\n \
    return r;\n \
}\n \
\n \
float float3_length(float3 a) {\n \
    return (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n \
}\n \
\n \
float float3_dot(float3 a, float3 b) {\n \
    return a.x * b.x + a.y * b.y + a.z * b.z;\n \
}\n \
\n \
float4 float4_null() {\n \
    float4 r;\n \
    r.x = 0.0f;\n \
    r.y = 0.0f;\n \
    r.z = 0.0f;\n \
    r.w = 0.0f;\n \
    return r;\n \
}\n \
\n \
float4 float4_make(float x, float y, float z, float w) {\n \
    float4 r;\n \
    r.x = x;\n \
    r.y = y;\n \
    r.z = z;\n \
    r.w = w;\n \
    return r;\n \
}\n \
\n \
float4 float4_add(float4 a, float4 b) {\n \
    float4 r;\n \
    r.x = a.x + b.x;\n \
    r.y = a.y + b.y;\n \
    r.z = a.z + b.z;\n \
    r.w = a.w + b.w;\n \
    return r;\n \
}\n \
\n \
float4 float4_sub(float4 a, float4 b) {\n \
    float4 r;\n \
    r.x = a.x - b.x;\n \
    r.y = a.y - b.y;\n \
    r.z = a.z - b.z;\n \
    r.w = a.w - b.w;\n \
    return r;\n \
}\n \
\n \
float4 float4_div_scalar(float4 a, float s) {\n \
    float4 r;\n \
    r.x = a.x / s;\n \
    r.y = a.y / s;\n \
    r.z = a.z / s;\n \
    r.w = a.w / s;\n \
    return r;\n \
}\n \
\n \
\n \
float4 float4_mul_scalar(float4 a, float s) {\n \
    float4 r;\n \
    r.x = a.x * s;\n \
    r.y = a.y * s;\n \
    r.z = a.z * s;\n \
    return r;\n \
}\n \
\n \
float4 float4_normalize(float4 a) {\n \
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n \
    float4 r;\n \
    if (l < 0.000001f) {\n \
        r.x = 1.0f;\n \
        r.y = 0.0f;\n \
        r.z = 0.0f;\n \
    }\n \
    else {\n \
        r.x = a.x / l;\n \
        r.y = a.y / l;\n \
        r.z = a.z / l;\n \
    }\n \
    return r;\n \
}\n \
\n \
float4 float4_normalize_to(float4 a, float b) {\n \
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n \
    float4 r;\n \
    if (l < 0.000001f) {\n \
        r.x = a.x;\n \
        r.y = a.y;\n \
        r.z = a.z;\n \
    }\n \
    else {\n \
        float c = b / l;\n \
        r.x = a.x * c;\n \
        r.y = a.y * c;\n \
        r.z = a.z * c;\n \
    }\n \
    return r;\n \
}\n \
\n \
float float4_length(float4 a) {\n \
    return (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);\n \
}\n \
\n \
float float4_dot(float4 a, float4 b) {\n \
    return a.x * b.x + a.y * b.y + a.z * b.z;\n \
}\n \
";

const char *agent_ocl_h = "\n \
typedef struct __attribute__((packed)) Agent {\n \
    TayAgentTag tag;\n \
    float4 p;\n \
    float4 dir;\n \
    unsigned color;\n \
    float speed;\n \
    float4 separation;\n \
    float4 alignment;\n \
    float4 cohesion;\n \
    int cohesion_count;\n \
    int separation_count;\n \
} Agent;\n \
\n \
typedef struct __attribute__((packed)) ActContext {\n \
    int dummy;\n \
} ActContext;\n \
\n \
typedef struct __attribute__((packed)) SeeContext {\n \
    float r;\n \
    float separation_r;\n \
} SeeContext;\n \
\n \
void agent_see(global Agent *a, global Agent *b, constant SeeContext *context);\n \
void agent_act(global Agent *a, constant ActContext *context);\n \
";

const char *taystd_ocl_h = "\n \
float3 float3_null();\n \
float3 float3_make(float x, float y, float z);\n \
float3 float3_add(float3 a, float3 b);\n \
float3 float3_sub(float3 a, float3 b);\n \
float3 float3_div_scalar(float3 a, float s);\n \
float3 float3_mul_scalar(float3 a, float s);\n \
float3 float3_normalize(float3 a);\n \
float3 float3_normalize_to(float3 a, float b);\n \
float float3_length(float3 a);\n \
float float3_dot(float3 a, float3 b);\n \
\n \
float4 float4_null();\n \
float4 float4_make(float x, float y, float z, float w);\n \
float4 float4_add(float4 a, float4 b);\n \
float4 float4_sub(float4 a, float4 b);\n \
float4 float4_div_scalar(float4 a, float s);\n \
float4 float4_mul_scalar(float4 a, float s);\n \
float4 float4_normalize(float4 a);\n \
float4 float4_normalize_to(float4 a, float b);\n \
float float4_length(float4 a);\n \
float float4_dot(float4 a, float4 b);\n \
";

