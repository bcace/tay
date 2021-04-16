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

typedef struct __PACK__ Particle {
    TayAgentTag tag;
    float4 p;
    float3 v;
    float3 f;
} Particle;

typedef struct __PACK__ ParticleSeeContext {
    float r;
} ParticleSeeContext;

typedef struct __PACK__ ParticleActContext {
    float3 min;
    float3 max;
} ParticleActContext;

void particle_see(__GLOBAL__ Particle *a, __GLOBAL__ Particle *b, __GLOBAL__ ParticleSeeContext *c);
void particle_act(__GLOBAL__ Particle *a, __GLOBAL__ ParticleActContext *c);

typedef struct __PACK__ Ball {
    TayAgentTag tag;
    float4 min;
    float4 max;
    float3 v;
    float3 f;
} Ball;

typedef struct __PACK__ BallParticleSeeContext {
    float r;
    float ball_r;
} BallParticleSeeContext;

void ball_act(__GLOBAL__ Ball *a, __GLOBAL__ void *c);
void ball_particle_see(__GLOBAL__ Ball *a, __GLOBAL__ Particle *b, __GLOBAL__ BallParticleSeeContext *c);
void particle_ball_see(__GLOBAL__ Particle *a, __GLOBAL__ Ball *b, __GLOBAL__ BallParticleSeeContext *c);

typedef struct __PACK__ SphParticle {
    TayAgentTag tag;
    float4 p;
    float3 vh;
    float3 v;
    float3 a;
    float density;
} SphParticle;

typedef struct __PACK__ SphContext {
    float h; /* smoothing (interaction) radius */
    float k; /* bulk modulus */
    float mu; /* viscosity */
    float rho0; /* reference density */
    float dt;
    float3 min;
    float3 max;

    float h2;
    float C;
    float C_own; // each particle's density must start with this value and then accumulate the neighbors' ones

    float C0;
    float Cp;
    float Cv;
} SphContext;

void sph_particle_density(__GLOBAL__ SphParticle *a, __GLOBAL__ SphParticle *b, __GLOBAL__ SphContext *c);
void sph_particle_acceleration(__GLOBAL__ SphParticle *a, __GLOBAL__ SphParticle *b, __GLOBAL__ SphContext *c);
void sph_particle_leapfrog(__GLOBAL__ SphParticle *a, __GLOBAL__ SphContext *c);
void sph_particle_reset(__GLOBAL__ SphParticle *a, __GLOBAL__ SphContext *c);
