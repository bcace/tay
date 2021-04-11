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
