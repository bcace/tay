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

    /* forces are only calculated for neighbors in front of the boid */

    float dot = float3_dot(a->dir, d);
    if (dot > 0.0f) {

        /* alignment */

        a->alignment = float3_add(a->alignment, b->dir);

        /* separation */

        if (dl < c->separation_r) {
            a->separation = float3_sub(a->separation, float3_normalize_to(d, c->separation_r - dl));
            ++a->separation_count;
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

    const float alignment_a = 0.02f;

    if (a->cohesion_count)
        acc = float3_add(acc, float3_mul_scalar(a->alignment, alignment_a / (float)a->cohesion_count));

    /* separation */

    const float separation_a = 0.02f;

    if (a->separation_count)
        acc = float3_add(acc, float3_mul_scalar(a->separation, separation_a / (float)a->separation_count));

    /* cohesion */

    const float cohesion_a = 0.001f;

    if (a->cohesion_count)
        acc = float3_add(acc, float3_mul_scalar(a->cohesion, cohesion_a / (float)a->cohesion_count));

    /* update */

    // const float min_speed = 0.2f;
    // const float max_speed = 0.4f;

    a->dir = float3_normalize(float3_add(a->dir, acc));
    // float dir_acc = float3_dot(a->dir, acc) * 0.01f;
    // a->speed += dir_acc;
    // if (a->speed < min_speed)
    //     a->speed = min_speed;
    // else if (a->speed > max_speed)
    //     a->speed = max_speed;
    float3_agent_position(a) = float3_add(p, float3_mul_scalar(a->dir, a->speed));

    a->separation = float3_null();
    a->alignment = float3_null();
    a->cohesion = float3_null();
    a->cohesion_count = 0;
    a->separation_count = 0;
}

void particle_see(Particle *a, Particle *b, ParticleSeeContext *c) {
    float3 d = float3_sub(b->p.xyz, a->p.xyz);
    float l = float3_length(d);
    if (l > 0.0f && l < c->r)
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - l) / l));
}

void particle_act(Particle *a, ParticleActContext *c) {

    const float m = 0.01f;
    const float t = 0.11f;

    float3 acc = float3_mul_scalar(a->f, 0.8f / m);                 // acceleration
    acc.z -= 30.0f;                                                 // gravity
    a->v = float3_add(a->v, float3_mul_scalar(acc, 0.5f * t));      // velocity increment
    a->p.xyz = float3_add(a->p.xyz, float3_mul_scalar(a->v, t));    // move
    a->v = float3_mul_scalar(a->v, 0.96f);                          // dissipate
    a->f = float3_null();                                           // reset force

    if (a->p.x < c->min.x) {
        a->p.x = c->min.x;
        a->v.x = -a->v.x;
    }

    if (a->p.y < c->min.y) {
        a->p.y = c->min.y;
        a->v.y = -a->v.y;
    }

    if (a->p.z < c->min.z) {
        a->p.z = c->min.z;
        a->v.z = -a->v.z;
    }

    if (a->p.x > c->max.x) {
        a->p.x = c->max.x;
        a->v.x = -a->v.x;
    }

    if (a->p.y > c->max.y) {
        a->p.y = c->max.y;
        a->v.y = -a->v.y;
    }

    if (a->p.z > c->max.z) {
        a->p.z = c->max.z;
        a->v.z = -a->v.z;
    }
}

void ball_particle_see(Ball *a, Particle *b, BallParticleSeeContext *c) {
    float3 a_p = float3_mul_scalar(float3_add(a->min.xyz, a->max.xyz), 0.5f);
    float3 d = float3_sub(b->p.xyz, a_p);
    float l = float3_length(d);
    float dl = l - c->ball_r;
    if (dl < c->r)
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - dl) / l));
}

void particle_ball_see(Particle *a, Ball *b, BallParticleSeeContext *c) {
    float3 b_p = float3_mul_scalar(float3_add(b->min.xyz, b->max.xyz), 0.5f);
    float3 d = float3_sub(b_p, a->p.xyz);
    float l = float3_length(d);
    float dl = l - c->ball_r;
    if (dl < c->r)
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - dl) / l));
}

void ball_act(Ball *a, void *c) {

    const float m = 40.1f;
    const float t = 0.11f; // TODO: move to context

    float3 acc = float3_mul_scalar(a->f, 0.8f / m);                 // acceleration
    acc.z -= 30.0f;                                                 // gravity
    a->v = float3_add(a->v, float3_mul_scalar(acc, 0.5f * t));      // velocity increment
    float3 d = float3_mul_scalar(a->v, t);
    a->min.xyz = float3_add(a->min.xyz, d);
    a->max.xyz = float3_add(a->max.xyz, d);
    a->v = float3_mul_scalar(a->v, 0.96f);                          // dissipate
    a->f = float3_null();                                           // reset force

    if (a->max.z < -100.0f) {
        a->min.z += 2000.0f;
        a->max.z += 2000.0f;
    }
}

void sph_particle_density(SphParticle *a, SphParticle *b, SphContext *c) {
    float dx = b->p.x - a->p.x;
    float dy = b->p.y - a->p.y;
    float dz = b->p.z - a->p.z;
    float r2 = dx * dx + dy * dy + dz * dz;
    float z = c->h2 - r2;
    if (z > 0.0f) // (1.f - clamp(floor(r / h), 0.f, 1.f))
        a->density += c->poly6 * z * z * z;
}

void sph_particle_acceleration(SphParticle *a, SphParticle *b, SphContext *c) {
    float3 r = float3_sub(b->p.xyz, a->p.xyz);
    float rl = float3_length(r);

    if (rl < c->h) {

        if (a != b) {

            float3 spiky_gradient;
            if (rl < 0.00001f) { // TODO: is this widening OK?
                spiky_gradient.x = c->spiky * c->h2;
                spiky_gradient.y = c->spiky * c->h2;
                spiky_gradient.z = c->spiky * c->h2;
            }
            else
                spiky_gradient = float3_mul_scalar(r, c->spiky * (c->h - rl) * (c->h - rl) / rl);

            a->pressure_accum = float3_add(
                                    a->pressure_accum,
                                    float3_mul(
                                        float3_add(
                                            float3_div_scalar(a->pressure, (a->density * a->density)),
                                            float3_div_scalar(b->pressure, (b->density * b->density))
                                        ),
                                        spiky_gradient
                                    )
                                );

            float viscosity_laplacian = c->viscosity * (c->h - rl);

            a->viscosity_accum = float3_add(a->viscosity_accum, float3_mul_scalar(float3_sub(b->v, a->v), viscosity_laplacian / b->density));
        }

        float3 poly6_gradient = float3_mul_scalar(r, c->poly6_gradient * (c->h2 - rl * rl) * (c->h2 - rl * rl) / b->density);

        a->normal = float3_add(a->normal, poly6_gradient);

        a->color_field_laplacian += c->poly6_laplacian * (c->h2 - rl * rl) * (3.0f * c->h2 - 7.0f * rl * rl) / b->density;
    }
}

void sph_particle_leapfrog(SphParticle *a, SphContext *c) {

    /* calculate internal forces */

    float3 f = float3_add(float3_mul_scalar(a->pressure_accum, -a->density), float3_mul_scalar(a->viscosity_accum, c->dynamic_viscosity));

    float nl = float3_length(a->normal);
    if (nl > c->surface_tension_threshold)
        f = float3_add(f, float3_mul_scalar(a->normal, -c->surface_tension * a->color_field_laplacian / nl));

    /* acceleration */

    float3 acc = float3_div_scalar(f, a->density);
    acc.z -= 9.81f;

    /* leapfrog */

    a->vh = float3_add(a->vh, float3_mul_scalar(acc, c->dt));
    a->v = float3_add(a->vh, float3_mul_scalar(acc, c->dt * 0.5f));
    a->p.xyz = float3_add(a->p.xyz, float3_mul_scalar(a->vh, c->dt));

    const float damp = 0.75f;

    if (a->p.x < c->min.x) {
        a->p.x = c->min.x;
        a->v.x = -a->v.x;
        a->vh.x = -a->vh.x;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.y < c->min.y) {
        a->p.y = c->min.y;
        a->v.y = -a->v.y;
        a->vh.y = -a->vh.y;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.z < c->min.z) {
        a->p.z = c->min.z;
        a->v.z = -a->v.z;
        a->vh.z = -a->vh.z;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.x > c->max.x) {
        a->p.x = c->max.x;
        a->v.x = -a->v.x;
        a->vh.x = -a->vh.x;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.y > c->max.y) {
        a->p.y = c->max.y;
        a->v.y = -a->v.y;
        a->vh.y = -a->vh.y;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.z > c->max.z) {
        a->p.z = c->max.z;
        a->v.z = -a->v.z;
        a->vh.z = -a->vh.z;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    sph_particle_reset(a);
}

void sph_particle_reset(SphParticle *a) {
    a->density = 0.0;
    a->color_field_laplacian = 0.0f;
    a->pressure_accum.x = 0.0f;
    a->pressure_accum.y = 0.0f;
    a->pressure_accum.z = 0.0f;
    a->viscosity_accum.x = 0.0f;
    a->viscosity_accum.y = 0.0f;
    a->viscosity_accum.z = 0.0f;
    a->normal.x = 0.0f;
    a->normal.y = 0.0f;
    a->normal.z = 0.0f;
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

float3 float3_mul(float3 a, float3 b) {
    float3 r;
    r.x = a.x * b.x;
    r.y = a.y * b.y;
    r.z = a.z * b.z;
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
    int separation_count;\n\
} Agent;\n\
\n\
typedef struct __attribute__((packed)) ActContext {\n\
    int dummy;\n\
} ActContext;\n\
\n\
typedef struct __attribute__((packed)) SeeContext {\n\
    float r;\n\
    float separation_r;\n\
} SeeContext;\n\
\n\
void agent_see(global Agent *a, global Agent *b, global SeeContext *context);\n\
void agent_act(global Agent *a, global ActContext *context);\n\
\n\
typedef struct __attribute__((packed)) Particle {\n\
    TayAgentTag tag;\n\
    float4 p;\n\
    float3 v;\n\
    float3 f;\n\
} Particle;\n\
\n\
typedef struct __attribute__((packed)) ParticleSeeContext {\n\
    float r;\n\
} ParticleSeeContext;\n\
\n\
typedef struct __attribute__((packed)) ParticleActContext {\n\
    float3 min;\n\
    float3 max;\n\
} ParticleActContext;\n\
\n\
void particle_see(global Particle *a, global Particle *b, global ParticleSeeContext *c);\n\
void particle_act(global Particle *a, global ParticleActContext *c);\n\
\n\
typedef struct __attribute__((packed)) Ball {\n\
    TayAgentTag tag;\n\
    float4 min;\n\
    float4 max;\n\
    float3 v;\n\
    float3 f;\n\
} Ball;\n\
\n\
typedef struct __attribute__((packed)) BallParticleSeeContext {\n\
    float r;\n\
    float ball_r;\n\
} BallParticleSeeContext;\n\
\n\
void ball_act(global Ball *a, global void *c);\n\
void ball_particle_see(global Ball *a, global Particle *b, global BallParticleSeeContext *c);\n\
void particle_ball_see(global Particle *a, global Ball *b, global BallParticleSeeContext *c);\n\
\n\
typedef struct __attribute__((packed)) SphParticle {\n\
    TayAgentTag tag;\n\
    float4 p;\n\
    float3 pressure;\n\
    float3 pressure_accum;\n\
    float3 viscosity_accum;\n\
    float3 normal;\n\
    float3 vh;\n\
    float3 v;\n\
    float density;\n\
    float color_field_laplacian;\n\
} SphParticle;\n\
\n\
typedef struct __attribute__((packed)) SphContext {\n\
    float h; /* smoothing (interaction) radius */\n\
    float dynamic_viscosity;\n\
    float dt;\n\
    float m;\n\
    float surface_tension;\n\
    float surface_tension_threshold;\n\
\n\
    float3 min;\n\
    float3 max;\n\
\n\
    float h2;\n\
    float poly6;\n\
    float poly6_gradient;\n\
    float poly6_laplacian;\n\
    float spiky;\n\
    float viscosity;\n\
} SphContext;\n\
\n\
void sph_particle_density(global SphParticle *a, global SphParticle *b, global SphContext *c);\n\
void sph_particle_acceleration(global SphParticle *a, global SphParticle *b, global SphContext *c);\n\
void sph_particle_leapfrog(global SphParticle *a, global SphContext *c);\n\
void sph_particle_reset(global SphParticle *a);\n\
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
float3 float3_mul(float3 a, float3 b) {\n\
    float3 r;\n\
    r.x = a.x * b.x;\n\
    r.y = a.y * b.y;\n\
    r.z = a.z * b.z;\n\
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
    /* forces are only calculated for neighbors in front of the boid */\n\
\n\
    float dot = float3_dot(a->dir, d);\n\
    if (dot > 0.0f) {\n\
\n\
        /* alignment */\n\
\n\
        a->alignment = float3_add(a->alignment, b->dir);\n\
\n\
        /* separation */\n\
\n\
        if (dl < c->separation_r) {\n\
            a->separation = float3_sub(a->separation, float3_normalize_to(d, c->separation_r - dl));\n\
            ++a->separation_count;\n\
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
    const float alignment_a = 0.02f;\n\
\n\
    if (a->cohesion_count)\n\
        acc = float3_add(acc, float3_mul_scalar(a->alignment, alignment_a / (float)a->cohesion_count));\n\
\n\
    /* separation */\n\
\n\
    const float separation_a = 0.02f;\n\
\n\
    if (a->separation_count)\n\
        acc = float3_add(acc, float3_mul_scalar(a->separation, separation_a / (float)a->separation_count));\n\
\n\
    /* cohesion */\n\
\n\
    const float cohesion_a = 0.001f;\n\
\n\
    if (a->cohesion_count)\n\
        acc = float3_add(acc, float3_mul_scalar(a->cohesion, cohesion_a / (float)a->cohesion_count));\n\
\n\
    /* update */\n\
\n\
    // const float min_speed = 0.2f;\n\
    // const float max_speed = 0.4f;\n\
\n\
    a->dir = float3_normalize(float3_add(a->dir, acc));\n\
    // float dir_acc = float3_dot(a->dir, acc) * 0.01f;\n\
    // a->speed += dir_acc;\n\
    // if (a->speed < min_speed)\n\
    //     a->speed = min_speed;\n\
    // else if (a->speed > max_speed)\n\
    //     a->speed = max_speed;\n\
    float3_agent_position(a) = float3_add(p, float3_mul_scalar(a->dir, a->speed));\n\
\n\
    a->separation = float3_null();\n\
    a->alignment = float3_null();\n\
    a->cohesion = float3_null();\n\
    a->cohesion_count = 0;\n\
    a->separation_count = 0;\n\
}\n\
\n\
void particle_see(global Particle *a, global Particle *b, global ParticleSeeContext *c) {\n\
    float3 d = float3_sub(b->p.xyz, a->p.xyz);\n\
    float l = float3_length(d);\n\
    if (l > 0.0f && l < c->r)\n\
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - l) / l));\n\
}\n\
\n\
void particle_act(global Particle *a, global ParticleActContext *c) {\n\
\n\
    const float m = 0.01f;\n\
    const float t = 0.11f;\n\
\n\
    float3 acc = float3_mul_scalar(a->f, 0.8f / m);                 // acceleration\n\
    acc.z -= 30.0f;                                                 // gravity\n\
    a->v = float3_add(a->v, float3_mul_scalar(acc, 0.5f * t));      // velocity increment\n\
    a->p.xyz = float3_add(a->p.xyz, float3_mul_scalar(a->v, t));    // move\n\
    a->v = float3_mul_scalar(a->v, 0.96f);                          // dissipate\n\
    a->f = float3_null();                                           // reset force\n\
\n\
    if (a->p.x < c->min.x) {\n\
        a->p.x = c->min.x;\n\
        a->v.x = -a->v.x;\n\
    }\n\
\n\
    if (a->p.y < c->min.y) {\n\
        a->p.y = c->min.y;\n\
        a->v.y = -a->v.y;\n\
    }\n\
\n\
    if (a->p.z < c->min.z) {\n\
        a->p.z = c->min.z;\n\
        a->v.z = -a->v.z;\n\
    }\n\
\n\
    if (a->p.x > c->max.x) {\n\
        a->p.x = c->max.x;\n\
        a->v.x = -a->v.x;\n\
    }\n\
\n\
    if (a->p.y > c->max.y) {\n\
        a->p.y = c->max.y;\n\
        a->v.y = -a->v.y;\n\
    }\n\
\n\
    if (a->p.z > c->max.z) {\n\
        a->p.z = c->max.z;\n\
        a->v.z = -a->v.z;\n\
    }\n\
}\n\
\n\
void ball_particle_see(global Ball *a, global Particle *b, global BallParticleSeeContext *c) {\n\
    float3 a_p = float3_mul_scalar(float3_add(a->min.xyz, a->max.xyz), 0.5f);\n\
    float3 d = float3_sub(b->p.xyz, a_p);\n\
    float l = float3_length(d);\n\
    float dl = l - c->ball_r;\n\
    if (dl < c->r)\n\
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - dl) / l));\n\
}\n\
\n\
void particle_ball_see(global Particle *a, global Ball *b, global BallParticleSeeContext *c) {\n\
    float3 b_p = float3_mul_scalar(float3_add(b->min.xyz, b->max.xyz), 0.5f);\n\
    float3 d = float3_sub(b_p, a->p.xyz);\n\
    float l = float3_length(d);\n\
    float dl = l - c->ball_r;\n\
    if (dl < c->r)\n\
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - dl) / l));\n\
}\n\
\n\
void ball_act(global Ball *a, global void *c) {\n\
\n\
    const float m = 40.1f;\n\
    const float t = 0.11f; // TODO: move to context\n\
\n\
    float3 acc = float3_mul_scalar(a->f, 0.8f / m);                 // acceleration\n\
    acc.z -= 30.0f;                                                 // gravity\n\
    a->v = float3_add(a->v, float3_mul_scalar(acc, 0.5f * t));      // velocity increment\n\
    float3 d = float3_mul_scalar(a->v, t);\n\
    a->min.xyz = float3_add(a->min.xyz, d);\n\
    a->max.xyz = float3_add(a->max.xyz, d);\n\
    a->v = float3_mul_scalar(a->v, 0.96f);                          // dissipate\n\
    a->f = float3_null();                                           // reset force\n\
\n\
    if (a->max.z < -100.0f) {\n\
        a->min.z += 2000.0f;\n\
        a->max.z += 2000.0f;\n\
    }\n\
}\n\
\n\
void sph_particle_density(global SphParticle *a, global SphParticle *b, global SphContext *c) {\n\
    float dx = b->p.x - a->p.x;\n\
    float dy = b->p.y - a->p.y;\n\
    float dz = b->p.z - a->p.z;\n\
    float r2 = dx * dx + dy * dy + dz * dz;\n\
    float z = c->h2 - r2;\n\
    if (z > 0.0f) // (1.f - clamp(floor(r / h), 0.f, 1.f))\n\
        a->density += c->poly6 * z * z * z;\n\
}\n\
\n\
void sph_particle_acceleration(global SphParticle *a, global SphParticle *b, global SphContext *c) {\n\
    float3 r = float3_sub(b->p.xyz, a->p.xyz);\n\
    float rl = float3_length(r);\n\
\n\
    if (rl < c->h) {\n\
\n\
        if (a != b) {\n\
\n\
            float3 spiky_gradient;\n\
            if (rl < 0.00001f) { // TODO: is this widening OK?\n\
                spiky_gradient.x = c->spiky * c->h2;\n\
                spiky_gradient.y = c->spiky * c->h2;\n\
                spiky_gradient.z = c->spiky * c->h2;\n\
            }\n\
            else\n\
                spiky_gradient = float3_mul_scalar(r, c->spiky * (c->h - rl) * (c->h - rl) / rl);\n\
\n\
            a->pressure_accum = float3_add(\n\
                                    a->pressure_accum,\n\
                                    float3_mul(\n\
                                        float3_add(\n\
                                            float3_div_scalar(a->pressure, (a->density * a->density)),\n\
                                            float3_div_scalar(b->pressure, (b->density * b->density))\n\
                                        ),\n\
                                        spiky_gradient\n\
                                    )\n\
                                );\n\
\n\
            float viscosity_laplacian = c->viscosity * (c->h - rl);\n\
\n\
            a->viscosity_accum = float3_add(a->viscosity_accum, float3_mul_scalar(float3_sub(b->v, a->v), viscosity_laplacian / b->density));\n\
        }\n\
\n\
        float3 poly6_gradient = float3_mul_scalar(r, c->poly6_gradient * (c->h2 - rl * rl) * (c->h2 - rl * rl) / b->density);\n\
\n\
        a->normal = float3_add(a->normal, poly6_gradient);\n\
\n\
        a->color_field_laplacian += c->poly6_laplacian * (c->h2 - rl * rl) * (3.0f * c->h2 - 7.0f * rl * rl) / b->density;\n\
    }\n\
}\n\
\n\
void sph_particle_leapfrog(global SphParticle *a, global SphContext *c) {\n\
\n\
    /* calculate internal forces */\n\
\n\
    float3 f = float3_add(float3_mul_scalar(a->pressure_accum, -a->density), float3_mul_scalar(a->viscosity_accum, c->dynamic_viscosity));\n\
\n\
    float nl = float3_length(a->normal);\n\
    if (nl > c->surface_tension_threshold)\n\
        f = float3_add(f, float3_mul_scalar(a->normal, -c->surface_tension * a->color_field_laplacian / nl));\n\
\n\
    /* acceleration */\n\
\n\
    float3 acc = float3_div_scalar(f, a->density);\n\
    acc.z -= 9.81f;\n\
\n\
    /* leapfrog */\n\
\n\
    a->vh = float3_add(a->vh, float3_mul_scalar(acc, c->dt));\n\
    a->v = float3_add(a->vh, float3_mul_scalar(acc, c->dt * 0.5f));\n\
    a->p.xyz = float3_add(a->p.xyz, float3_mul_scalar(a->vh, c->dt));\n\
\n\
    const float damp = 0.75f;\n\
\n\
    if (a->p.x < c->min.x) {\n\
        a->p.x = c->min.x;\n\
        a->v.x = -a->v.x;\n\
        a->vh.x = -a->vh.x;\n\
        // a->v = float3_mul_scalar(a->v, damp);\n\
        // a->vh = float3_mul_scalar(a->vh, damp);\n\
    }\n\
\n\
    if (a->p.y < c->min.y) {\n\
        a->p.y = c->min.y;\n\
        a->v.y = -a->v.y;\n\
        a->vh.y = -a->vh.y;\n\
        // a->v = float3_mul_scalar(a->v, damp);\n\
        // a->vh = float3_mul_scalar(a->vh, damp);\n\
    }\n\
\n\
    if (a->p.z < c->min.z) {\n\
        a->p.z = c->min.z;\n\
        a->v.z = -a->v.z;\n\
        a->vh.z = -a->vh.z;\n\
        // a->v = float3_mul_scalar(a->v, damp);\n\
        // a->vh = float3_mul_scalar(a->vh, damp);\n\
    }\n\
\n\
    if (a->p.x > c->max.x) {\n\
        a->p.x = c->max.x;\n\
        a->v.x = -a->v.x;\n\
        a->vh.x = -a->vh.x;\n\
        // a->v = float3_mul_scalar(a->v, damp);\n\
        // a->vh = float3_mul_scalar(a->vh, damp);\n\
    }\n\
\n\
    if (a->p.y > c->max.y) {\n\
        a->p.y = c->max.y;\n\
        a->v.y = -a->v.y;\n\
        a->vh.y = -a->vh.y;\n\
        // a->v = float3_mul_scalar(a->v, damp);\n\
        // a->vh = float3_mul_scalar(a->vh, damp);\n\
    }\n\
\n\
    if (a->p.z > c->max.z) {\n\
        a->p.z = c->max.z;\n\
        a->v.z = -a->v.z;\n\
        a->vh.z = -a->vh.z;\n\
        // a->v = float3_mul_scalar(a->v, damp);\n\
        // a->vh = float3_mul_scalar(a->vh, damp);\n\
    }\n\
\n\
    sph_particle_reset(a);\n\
}\n\
\n\
void sph_particle_reset(global SphParticle *a) {\n\
    a->density = 0.0;\n\
    a->color_field_laplacian = 0.0f;\n\
    a->pressure_accum.x = 0.0f;\n\
    a->pressure_accum.y = 0.0f;\n\
    a->pressure_accum.z = 0.0f;\n\
    a->viscosity_accum.x = 0.0f;\n\
    a->viscosity_accum.y = 0.0f;\n\
    a->viscosity_accum.z = 0.0f;\n\
    a->normal.x = 0.0f;\n\
    a->normal.y = 0.0f;\n\
    a->normal.z = 0.0f;\n\
}\n\
\n\
";
