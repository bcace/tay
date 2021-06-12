
typedef struct __attribute__((packed)) Agent {
    TayAgentTag tag;
    float4 p;
    float4 dir;
    float speed;
    float4 separation;
    float4 alignment;
    float4 cohesion;
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

typedef struct __attribute__((packed)) PicBoid {
    TayAgentTag tag;
    float4 p;
    float4 dir;
    float4 dir_sum;
} PicBoid;

typedef struct __attribute__((packed)) PicBoidNode {
    float4 p;
    float4 dir_sum;
} PicBoidNode;

typedef struct __attribute__((packing)) PicFlockingContext {
    float radius;
    float4 min;
    float4 max;
} PicFlockingContext;

void pic_reset_node(global PicBoidNode *n, global void *c);
void pic_transfer_boid_to_node(global PicBoid *a, global PicBoidNode *n, constant PicFlockingContext *c);
void pic_transfer_node_to_boids(global PicBoid *a, global PicBoidNode *n, constant PicFlockingContext *c);
void pic_boid_action(global PicBoid *a, constant PicFlockingContext *c);
