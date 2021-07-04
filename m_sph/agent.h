
typedef struct __attribute__((packed)) SphParticle {
    TayAgentTag tag;
    float4 p;
    float4 pressure_accum;
    float4 viscosity_accum;
    float4 vh;
    float4 v;
    float density;
    float pressure;
} SphParticle;

typedef struct __attribute__((packed)) SphContext {
    float h; /* smoothing (interaction) radius */
    float dynamic_viscosity;
    float dt;
    float m;
    float surface_tension;
    float surface_tension_threshold;
    float K;
    float density;

    float4 min;
    float4 max;

    float h2;
    float poly6;
    float poly6_gradient;
    float poly6_laplacian;
    float spiky;
    float viscosity;
} SphContext;

void sph_particle_density(global SphParticle *a, global SphParticle *b, constant SphContext *c);
void sph_particle_pressure(global SphParticle *a, constant SphContext *c);
void sph_force_terms(global SphParticle *a, global SphParticle *b, constant SphContext *c);
void sph_particle_leapfrog(global SphParticle *a, constant SphContext *c);
void sph_particle_reset(global SphParticle *a);
