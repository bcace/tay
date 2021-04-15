#include "main.h"
#include "tay.h"
#include "agent.h"
#include "shaders.h"
#include "graphics.h"
#include <math.h>
#include <stdlib.h>


static Program program;

static TayGroup *particles_group;
static TayGroup *balls_group;

static BallParticleSeeContext ball_particle_see_context;
static ParticleSeeContext particle_see_context;
static ParticleActContext particle_act_context;

static int particles_count = 40000;
static int balls_count = 2;
static float ball_r = 50.0f;

static float sphere[10000];
static unsigned sphere_subdivs = 2;

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

static void _init_sph_context(SphContext *c, float h, float m, float k, float mu, float rho0, float dt) {
    c->h = h;
    c->k = k;
    c->mu = mu;
    c->rho0 = rho0;
    c->dt = dt;

    c->h2 = h * h;

    /* density: Bindel, Fall (2011) */
    c->C = 4.0f * m / (3.1415926536f * c->h2 * c->h2 * c->h2 * c->h2);
    c->C_own = 4.0f * m / (3.1415926536f * c->h2);

    /* density: Matthias Müller, David Charypar and Markus Gross (2003) */
    // c->C = 315.0f * m / (64.0f * 3.1415926536f * c->h2 * c->h2 * c->h2 * c->h2 * h);
    // c->C_own = 315.0f * m / (64.0f * 3.1415926536f * c->h2 * h);

    /* acceleration: Bindel, Fall (2011) */
    c->C0 = m / (3.1415926536f * c->h2 * c->h2);
    c->Cp = 15.0f * k;
    c->Cv = -40.0f * mu;
}

void fluid_init() {
    const float r = 10.0f;
    const float part_r = r * 0.5f;

    icosahedron_verts(sphere_subdivs, sphere);

    particles_group = tay_add_group(global.tay, sizeof(Particle), particles_count, TAY_TRUE);
    tay_configure_space(global.tay, particles_group, TAY_CPU_GRID, 3, (float4){part_r, part_r, part_r, part_r}, 250);

    balls_group = tay_add_group(global.tay, sizeof(Ball), balls_count, TAY_FALSE);
    tay_configure_space(global.tay, balls_group, TAY_CPU_AABB_TREE, 3, (float4){part_r, part_r, part_r, part_r}, 250);

    particle_see_context.r = r;

    particle_act_context.min = (float3){-300.0f, -300.0f, -50.0f};
    particle_act_context.max = (float3){300.0f, 300.0f, 300.0f};

    ball_particle_see_context.r = r;
    ball_particle_see_context.ball_r = ball_r;

    tay_add_see(global.tay, particles_group, particles_group, particle_see, (float4){r, r, r, r}, &particle_see_context);
    tay_add_see(global.tay, balls_group, particles_group, ball_particle_see, (float4){r, r, r, r}, &ball_particle_see_context);
    tay_add_see(global.tay, particles_group, balls_group, particle_ball_see, (float4){r, r, r, r}, &ball_particle_see_context);

    tay_add_act(global.tay, balls_group, ball_act, &particle_act_context);
    tay_add_act(global.tay, particles_group, particle_act, &particle_act_context);

    for (int i = 0; i < particles_count; ++i) {
        Particle *p = tay_get_available_agent(global.tay, particles_group);
        p->p.x = _rand(particle_act_context.min.x, particle_act_context.max.x);
        p->p.y = _rand(particle_act_context.min.y, particle_act_context.max.y);
        p->p.z = _rand(0.0f, 100.0f);
        p->v = (float3){0.0f, 0.0f, 0.0f};
        p->f = (float3){0.0f, 0.0f, 0.0f};
        tay_commit_available_agent(global.tay, particles_group);
    }

    for (int i = 0; i < balls_count; ++i) {
        Ball *b = tay_get_available_agent(global.tay, balls_group);
        float x = -150.0f + i * 300.0f;
        float y = -150.0f + i * 300.0f;
        float z = 1000.0f + i * 200.0f;
        b->min.x = x - ball_r;
        b->min.y = y - ball_r;
        b->min.z = z - ball_r;
        b->max.x = x + ball_r;
        b->max.y = y + ball_r;
        b->max.z = z + ball_r;
        b->v = (float3){0.0f, 0.0f, 0.0f};
        b->f = (float3){0.0f, 0.0f, 0.0f};
        tay_commit_available_agent(global.tay, balls_group);
    }

    /* drawing init */

    shader_program_init(&program, particles_vert, "particles.vert", particles_frag, "particles.frag");
    shader_program_define_in_float(&program, 3); /* vertex position */
    shader_program_define_instanced_in_float(&program, 3); /* instance position */
    shader_program_define_instanced_in_float(&program, 1); /* instance size */
    shader_program_define_instanced_in_float(&program, 1); /* instance energy */
    shader_program_define_uniform(&program, "projection");
}

void fluid_draw() {
    mat4 perspective;
    graphics_perspective(&perspective, 1.2f, (float)global.window_w / (float)global.window_h, 1.0f, 2000.0f);

    mat4 modelview;
    mat4_set_identity(&modelview);
    mat4_translate(&modelview, 0.0f, 50.0f, -800.0f);
    mat4_rotate(&modelview, -0.8f, 1.0f, 0.0f, 0.0f);
    // mat4_rotate(&modelview, 0.7f, 0.0f, 0.0f, 1.0f);

    mat4 projection;
    mat4_multiply(&projection, &perspective, &modelview);

    shader_program_use(&program);
    shader_program_set_uniform_mat4(&program, 0, &projection);

    shader_program_set_data_float(&program, 0, CUBE_VERTS_COUNT, 3, CUBE_VERTS);

    vec3 *inst_pos = global.inst_vec3_buffers[0];
    float *inst_size = global.inst_float_buffers[0];
    float *inst_energy = global.inst_float_buffers[1];

    for (int i = 0; i < particles_count; ++i) {
        Particle *p = tay_get_agent(global.tay, particles_group, i);
        inst_pos[i].x = p->p.x;
        inst_pos[i].y = p->p.y;
        inst_pos[i].z = p->p.z;
        inst_size[i] = 4.0f;
        inst_energy[i] = sqrtf(p->v.x * p->v.x + p->v.y * p->v.y + p->v.z * p->v.z) * 0.1f;
    }

    shader_program_set_data_float(&program, 1, particles_count, 3, inst_pos);
    shader_program_set_data_float(&program, 2, particles_count, 1, inst_size);
    shader_program_set_data_float(&program, 3, particles_count, 1, inst_energy);

    graphics_draw_triangles_instanced(CUBE_VERTS_COUNT, particles_count);

    shader_program_set_data_float(&program, 0, icosahedron_verts_count(sphere_subdivs), 3, sphere);

    for (int i = 0; i < balls_count; ++i) {
        Ball *b = tay_get_agent(global.tay, balls_group, i);
        inst_pos[i].x = (b->min.x + b->max.x) * 0.5f;
        inst_pos[i].y = (b->min.y + b->max.y) * 0.5f;
        inst_pos[i].z = (b->min.z + b->max.z) * 0.5f;
        inst_size[i] = ball_r;
        inst_energy[i] = 0.0f;
    }

    shader_program_set_data_float(&program, 1, balls_count, 3, inst_pos);
    shader_program_set_data_float(&program, 2, balls_count, 1, inst_size);
    shader_program_set_data_float(&program, 3, balls_count, 1, inst_energy);

    graphics_draw_triangles_instanced(icosahedron_verts_count(sphere_subdivs), balls_count);
}
