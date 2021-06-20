
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
    float4 separation;
    float4 alignment;
    float4 cohesion;
    int cohesion_count;
    int separation_count;
} PicBoid;

typedef struct __attribute__((packed)) PicBoidNode {
    float4 p;
    float4 p_sum;
    float4 dir_sum;
    float4 w_sum;
} PicBoidNode;

typedef struct __attribute__((packing)) PicFlockingContext {
    float r;
    float separation_r;
    float cell_size;
    float4 min;
    float4 max;
} PicFlockingContext;

void pic_reset_node(global PicBoidNode *n, global void *c);
void pic_transfer_boid_to_node(global PicBoid *a, global PicBoidNode *n, constant PicFlockingContext *c);
void pic_normalize_node(global PicBoidNode *n, constant PicFlockingContext *c);
void pic_transfer_node_to_boids(global PicBoid *a, global PicBoidNode *n, constant PicFlockingContext *c);
void pic_boid_action(global PicBoid *a, constant PicFlockingContext *c);

typedef struct __attribute__((packing))  Taichi2DParticle {
    TayAgentTag tag;
    float4 p;
    float2 v;
    float4 F; // Deformation gradient
    float4 C; // Affine momentum from APIC
    float Jp; // Determinant of the deformation gradient (i.e. volume)
    int color;
} Taichi2DParticle;

typedef struct __attribute__((packing)) Taichi2DNode {
    float4 p;
    float2 v;
    float m;
} Taichi2DNode;

typedef struct __attribute__((packing)) Taichi2DContext {
    float dt;
} Taichi2DContext;

/* host only */
void taichi_2D_init_particle(Taichi2DParticle *p);

void taichi_2D_reset_node(global Taichi2DNode *n, constant Taichi2DContext *c);
void taichi_2D_particle_to_node(global Taichi2DParticle *p, global TayPicKernel *k, constant Taichi2DContext *c);
void taichi_2D_node(global Taichi2DNode *n, constant Taichi2DContext *c);
void taichi_2D_node_to_particle(global Taichi2DParticle *p, global TayPicKernel *k, constant Taichi2DContext *c);
