
typedef struct __attribute__((packed)) Agent {
    TayAgentTag tag;
    float4 p;
    float3 dir;
    float speed;
    float3 separation;
    float3 alignment;
    float3 cohesion;
    int cohesion_count;
    int separation_count;
} Agent;

typedef struct __attribute__((packed)) ActContext {
    int dummy;
} ActContext;

typedef struct __attribute__((packed)) SeeContext {
    float r;
    float separation_r;
} SeeContext;

void agent_see(global Agent *a, global Agent *b, constant SeeContext *context);
void agent_act(global Agent *a, constant ActContext *context);

typedef struct __attribute__((packed)) Ball {
    TayAgentTag tag;
    float4 min;
    float4 max;
    float3 v;
    float3 f;
} Ball;

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
