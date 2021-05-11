
typedef struct Agent {
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

typedef struct ActContext {
    int dummy;
} ActContext;

typedef struct SeeContext {
    float r;
    float separation_r;
} SeeContext;

void agent_see(Agent *a, Agent *b, SeeContext *context);
void agent_act(Agent *a, ActContext *context);

typedef struct Ball {
    TayAgentTag tag;
    float4 min;
    float4 max;
    float3 v;
    float3 f;
} Ball;

typedef struct SphParticle {
    TayAgentTag tag;
    float4 p;
    float3 pressure_accum;
    float3 viscosity_accum;
    float3 vh;
    float3 v;
    float density;
    float pressure;
} SphParticle;

typedef struct SphContext {
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

void sph_particle_density(SphParticle *a, SphParticle *b, SphContext *c);
void sph_particle_pressure(SphParticle *a, SphContext *c);
void sph_force_terms(SphParticle *a, SphParticle *b, SphContext *c);
void sph_particle_leapfrog(SphParticle *a, SphContext *c);
void sph_particle_reset(SphParticle *a);
