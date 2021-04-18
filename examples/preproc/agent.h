typedef struct __PACK__ Agent {
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

typedef struct __PACK__ ActContext {
    int dummy;
} ActContext;

typedef struct __PACK__ SeeContext {
    float r;
    float separation_r;
} SeeContext;

void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *context);
void agent_act(__GLOBAL__ Agent *a, __GLOBAL__ ActContext *context);

typedef struct __PACK__ Ball {
    TayAgentTag tag;
    float4 min;
    float4 max;
    float3 v;
    float3 f;
} Ball;

typedef struct __PACK__ SphParticle {
    TayAgentTag tag;
    float4 p;
    float3 pressure_accum;
    float3 viscosity_accum;
    float3 vh;
    float3 v;
    float density;
    float pressure;
} SphParticle;

typedef struct __PACK__ SphContext {
    float h; /* smoothing (interaction) radius */
    float dynamic_viscosity;
    float dt;
    float m;
    float surface_tension;
    float surface_tension_threshold;
    float K;
    float density;

    float3 min;
    float3 max;

    float h2;
    float poly6;
    float poly6_gradient;
    float poly6_laplacian;
    float spiky;
    float viscosity;
} SphContext;

void sph_particle_density(__GLOBAL__ SphParticle *a, __GLOBAL__ SphParticle *b, __GLOBAL__ SphContext *c);
void sph_particle_pressure(__GLOBAL__ SphParticle *a, __GLOBAL__ SphContext *c);
void sph_force_terms(__GLOBAL__ SphParticle *a, __GLOBAL__ SphParticle *b, __GLOBAL__ SphContext *c);
void sph_particle_leapfrog(__GLOBAL__ SphParticle *a, __GLOBAL__ SphContext *c);
void sph_particle_reset(__GLOBAL__ SphParticle *a);
