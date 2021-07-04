const char *agent_ocl_c = "\n \
void agent_see(global Agent *a, global Agent *b, constant SeeContext *c) {\n \
    float4 a_p = float4_agent_position(a);\n \
    float4 b_p = float4_agent_position(b);\n \
    for (int i = 0; i < 1; ++i)\n \
        a->b_buffer = float4_add(a->b_buffer, float4_sub(b_p, a_p));\n \
    a->b_buffer_count++;\n \
}\n \
\n \
void agent_act(global Agent *agent, constant ActContext *c) {\n \
\n \
    /* buffer swap */\n \
\n \
    if (agent->b_buffer_count != 0) {\n \
        float4 n = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);\n \
        agent->f_buffer = float4_add(agent->f_buffer, n);\n \
        agent->b_buffer = float4_null();\n \
        agent->b_buffer_count = 0;\n \
    }\n \
\n \
    /* move */\n \
\n \
    float4 p = float4_agent_position(agent);\n \
    float4_agent_position(agent) = float4_add(p, agent->v);\n \
\n \
    if (agent->p.x < c->min.x) {\n \
        agent->p.x = c->min.x;\n \
        agent->v.x = -agent->v.x;\n \
    }\n \
\n \
    if (agent->p.y < c->min.y) {\n \
        agent->p.y = c->min.y;\n \
        agent->v.y = -agent->v.y;\n \
    }\n \
\n \
    if (agent->p.z < c->min.z) {\n \
        agent->p.z = c->min.z;\n \
        agent->v.z = -agent->v.z;\n \
    }\n \
\n \
    if (agent->p.x > c->max.x) {\n \
        agent->p.x = c->max.x;\n \
        agent->v.x = -agent->v.x;\n \
    }\n \
\n \
    if (agent->p.y > c->max.y) {\n \
        agent->p.y = c->max.y;\n \
        agent->v.y = -agent->v.y;\n \
    }\n \
\n \
    if (agent->p.z > c->max.z) {\n \
        agent->p.z = c->max.z;\n \
        agent->v.z = -agent->v.z;\n \
    }\n \
}\n \
\n \
void box_agent_see(global BoxAgent *a, global BoxAgent *b, constant SeeContext *c) {\n \
    float4 a_min = float4_agent_min(a);\n \
    float4 b_min = float4_agent_min(b);\n \
    for (int i = 0; i < 1; ++i)\n \
        a->b_buffer = float4_add(a->b_buffer, float4_sub(b_min, a_min));\n \
    a->b_buffer_count++;\n \
}\n \
\n \
void box_agent_act(global BoxAgent *agent, constant ActContext *c) {\n \
\n \
    /* buffer swap */\n \
\n \
    if (agent->b_buffer_count != 0) {\n \
        float4 n = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);\n \
        agent->f_buffer = float4_add(agent->f_buffer, n);\n \
        agent->b_buffer = float4_null();\n \
        agent->b_buffer_count = 0;\n \
    }\n \
\n \
    /* move */\n \
\n \
    float4 min = float4_agent_min(agent);\n \
    float4 max = float4_agent_max(agent);\n \
    float4 size = float4_sub(max, min);\n \
    float4_agent_min(agent) = float4_add(min, agent->v);\n \
    float4_agent_max(agent) = float4_add(max, agent->v);\n \
\n \
    if (agent->min.x < c->min.x) {\n \
        agent->min.x = c->min.x;\n \
        agent->max.x = c->min.x + size.x;\n \
        agent->v.x = -agent->v.x;\n \
    }\n \
\n \
    if (agent->min.y < c->min.y) {\n \
        agent->min.y = c->min.y;\n \
        agent->max.y = c->min.y + size.y;\n \
        agent->v.y = -agent->v.y;\n \
    }\n \
\n \
    if (agent->min.z < c->min.z) {\n \
        agent->min.z = c->min.z;\n \
        agent->max.z = c->min.z + size.z;\n \
        agent->v.z = -agent->v.z;\n \
    }\n \
\n \
    if (agent->max.x > c->max.x) {\n \
        agent->max.x = c->max.x;\n \
        agent->min.x = c->max.x - size.x;\n \
        agent->v.x = -agent->v.x;\n \
    }\n \
\n \
    if (agent->max.y > c->max.y) {\n \
        agent->max.y = c->max.y;\n \
        agent->min.y = c->max.y - size.y;\n \
        agent->v.y = -agent->v.y;\n \
    }\n \
\n \
    if (agent->max.z > c->max.z) {\n \
        agent->max.z = c->max.z;\n \
        agent->min.z = c->max.z - size.z;\n \
        agent->v.z = -agent->v.z;\n \
    }\n \
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
    return r;\n \
}\n \
\n \
float4 float4_make(float x, float y, float z, float w) {\n \
    float4 r;\n \
    r.x = x;\n \
    r.y = y;\n \
    r.z = z;\n \
    return r;\n \
}\n \
\n \
float4 float4_add(float4 a, float4 b) {\n \
    float4 r;\n \
    r.x = a.x + b.x;\n \
    r.y = a.y + b.y;\n \
    r.z = a.z + b.z;\n \
    return r;\n \
}\n \
\n \
float4 float4_sub(float4 a, float4 b) {\n \
    float4 r;\n \
    r.x = a.x - b.x;\n \
    r.y = a.y - b.y;\n \
    r.z = a.z - b.z;\n \
    return r;\n \
}\n \
\n \
float4 float4_div_scalar(float4 a, float s) {\n \
    float4 r;\n \
    r.x = a.x / s;\n \
    r.y = a.y / s;\n \
    r.z = a.z / s;\n \
    return r;\n \
}\n \
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
    float4 v;\n \
    float4 b_buffer;\n \
    int b_buffer_count;\n \
    float4 f_buffer;\n \
    int result_index;\n \
} Agent;\n \
\n \
typedef struct __attribute__((packed)) ActContext {\n \
    float4 min;\n \
    float4 max;\n \
} ActContext;\n \
\n \
typedef struct __attribute__((packed)) SeeContext {\n \
    float4 radii;\n \
} SeeContext;\n \
\n \
typedef struct __attribute__((packed)) BoxAgent {\n \
    TayAgentTag tag;\n \
    float4 min;\n \
    float4 max;\n \
    float4 v;\n \
    float4 b_buffer;\n \
    int b_buffer_count;\n \
    float4 f_buffer;\n \
    int result_index;\n \
} BoxAgent;\n \
\n \
void agent_see(global Agent *a, global Agent *b, constant SeeContext *context);\n \
void agent_act(global Agent *agent, constant ActContext *context);\n \
\n \
void box_agent_see(global BoxAgent *a, global BoxAgent *b, constant SeeContext *context);\n \
void box_agent_act(global BoxAgent *agent, constant ActContext *context);\n \
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

