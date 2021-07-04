const char *agent_ocl_c = "\n \
void sph_particle_density(global SphParticle *a, global SphParticle *b, constant SphContext *c) {\n \
    float dx = b->p.x - a->p.x;\n \
    float dy = b->p.y - a->p.y;\n \
    float dz = b->p.z - a->p.z;\n \
    float r2 = dx * dx + dy * dy + dz * dz;\n \
    float z = c->h2 - r2;\n \
    if (z > 0.0f) // (1.f - clamp(floor(r / h), 0.f, 1.f))\n \
        a->density += c->poly6 * z * z * z;\n \
}\n \
\n \
void sph_particle_pressure(global SphParticle *a, constant SphContext *c) {\n \
    /* Tait equation more suitable to liquids than state equation */\n \
    a->pressure = c->K * ((float)pow(a->density / c->density, 7.0f) - 1.0f);\n \
\n \
    /* state equation */\n \
    // a->pressure = c->K * (a->density - c->density);\n \
}\n \
\n \
void sph_force_terms(global SphParticle *a, global SphParticle *b, constant SphContext *c) {\n \
    float4 r;\n \
    r.x = b->p.x - a->p.x;\n \
    r.y = b->p.y - a->p.y;\n \
    r.z = b->p.z - a->p.z;\n \
    float r2 = r.x * r.x + r.y * r.y + r.z * r.z;\n \
\n \
    float rl = (float)sqrt(r2);\n \
\n \
    if (rl < c->h) {\n \
\n \
        float spiky_gradient;\n \
        if (rl < 0.00001f)\n \
            spiky_gradient = c->spiky * c->h2;\n \
        else\n \
            spiky_gradient = c->spiky * (c->h - rl) * (c->h - rl);\n \
\n \
        if (rl > 0.00001f)\n \
            a->pressure_accum = float4_add(a->pressure_accum, float4_mul_scalar(r, spiky_gradient * (a->pressure + b->pressure) / (2.0f * b->density) / rl));\n \
\n \
        a->viscosity_accum = float4_add(a->viscosity_accum, float4_mul_scalar(float4_sub(b->v, a->v), c->viscosity * (c->h - rl) / b->density));\n \
    }\n \
}\n \
\n \
void sph_particle_leapfrog(global SphParticle *a, constant SphContext *c) {\n \
\n \
    /* calculate internal forces */\n \
\n \
    float4 f = float4_add(a->pressure_accum, float4_mul_scalar(a->viscosity_accum, c->dynamic_viscosity));\n \
\n \
    /* acceleration */\n \
\n \
    float4 acc = float4_div_scalar(f, a->density);\n \
    acc.z -= 9.81f;\n \
\n \
    /* leapfrog */\n \
\n \
    a->vh = float4_add(a->vh, float4_mul_scalar(acc, c->dt));\n \
\n \
    a->v = float4_add(a->vh, float4_mul_scalar(acc, c->dt * 0.5f));\n \
    a->p = float4_add(a->p, float4_mul_scalar(a->vh, c->dt));\n \
\n \
    const float damp = -0.5f;\n \
\n \
    if (a->p.x < c->min.x) {\n \
        a->p.x = c->min.x;\n \
        a->v.x = damp * a->v.x;\n \
        a->vh.x = damp * a->vh.x;\n \
    }\n \
\n \
    if (a->p.y < c->min.y) {\n \
        a->p.y = c->min.y;\n \
        a->v.y = damp * a->v.y;\n \
        a->vh.y = damp * a->vh.y;\n \
    }\n \
\n \
    if (a->p.z < c->min.z) {\n \
        a->p.z = c->min.z;\n \
        a->v.z = damp * a->v.z;\n \
        a->vh.z = damp * a->vh.z;\n \
    }\n \
\n \
    if (a->p.x > c->max.x) {\n \
        a->p.x = c->max.x;\n \
        a->v.x = damp * a->v.x;\n \
        a->vh.x = damp * a->vh.x;\n \
    }\n \
\n \
    if (a->p.y > c->max.y) {\n \
        a->p.y = c->max.y;\n \
        a->v.y = damp * a->v.y;\n \
        a->vh.y = damp * a->vh.y;\n \
    }\n \
\n \
    if (a->p.z > c->max.z) {\n \
        a->p.z = c->max.z;\n \
        a->v.z = damp * a->v.z;\n \
        a->vh.z = damp * a->vh.z;\n \
    }\n \
\n \
    sph_particle_reset(a);\n \
}\n \
\n \
void sph_particle_reset(global SphParticle *a) {\n \
    a->density = 0.0;\n \
    a->pressure_accum.x = 0.0f;\n \
    a->pressure_accum.y = 0.0f;\n \
    a->pressure_accum.z = 0.0f;\n \
    a->viscosity_accum.x = 0.0f;\n \
    a->viscosity_accum.y = 0.0f;\n \
    a->viscosity_accum.z = 0.0f;\n \
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
typedef struct __attribute__((packed)) SphParticle {\n \
    TayAgentTag tag;\n \
    float4 p;\n \
    float4 pressure_accum;\n \
    float4 viscosity_accum;\n \
    float4 vh;\n \
    float4 v;\n \
    float density;\n \
    float pressure;\n \
} SphParticle;\n \
\n \
typedef struct __attribute__((packed)) SphContext {\n \
    float h; /* smoothing (interaction) radius */\n \
    float dynamic_viscosity;\n \
    float dt;\n \
    float m;\n \
    float surface_tension;\n \
    float surface_tension_threshold;\n \
    float K;\n \
    float density;\n \
\n \
    float4 min;\n \
    float4 max;\n \
\n \
    float h2;\n \
    float poly6;\n \
    float poly6_gradient;\n \
    float poly6_laplacian;\n \
    float spiky;\n \
    float viscosity;\n \
} SphContext;\n \
\n \
void sph_particle_density(global SphParticle *a, global SphParticle *b, constant SphContext *c);\n \
void sph_particle_pressure(global SphParticle *a, constant SphContext *c);\n \
void sph_force_terms(global SphParticle *a, global SphParticle *b, constant SphContext *c);\n \
void sph_particle_leapfrog(global SphParticle *a, constant SphContext *c);\n \
void sph_particle_reset(global SphParticle *a);\n \
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

