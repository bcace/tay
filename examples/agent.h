#ifndef tay_agent_h
#define tay_agent_h

#include "tay.h"


#pragma pack(push, 1)

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

typedef struct Particle {
    TayAgentTag tag;
    float4 p;
    float3 v;
    float3 f;
} Particle;

typedef struct ParticleSeeContext {
    float r;
} ParticleSeeContext;

typedef struct ParticleActContext {
    float3 min;
    float3 max;
} ParticleActContext;

void particle_see(Particle *a, Particle *b, ParticleSeeContext *c);
void particle_act(Particle *a, ParticleActContext *c);

typedef struct Ball {
    TayAgentTag tag;
    float4 min;
    float4 max;
    float3 v;
    float3 f;
} Ball;

typedef struct BallParticleSeeContext {
    float r;
    float ball_r;
} BallParticleSeeContext;

void ball_act(Ball *a, void *c);
void ball_particle_see(Ball *a, Particle *b, BallParticleSeeContext *c);
void particle_ball_see(Particle *a, Ball *b, BallParticleSeeContext *c);


float3 float3_null();
float3 float3_make(float x, float y, float z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_div_scalar(float3 a, float s);
float3 float3_mul_scalar(float3 a, float s);
float3 float3_normalize(float3 a);
float3 float3_normalize_to(float3 a, float b);
float float3_length(float3 a);
float float3_dot(float3 a, float3 b);

float4 float4_null();
float4 float4_make(float x, float y, float z, float w);
float4 float4_add(float4 a, float4 b);
float4 float4_sub(float4 a, float4 b);
float4 float4_div_scalar(float4 a, float s);

extern const char *agent_kernels_source;

#pragma pack(pop)

#endif
