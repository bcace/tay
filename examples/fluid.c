#include "main.h"
#include "tay.h"
#include "agent.h"
#include "shaders.h"
#include "graphics.h"
#include <math.h>
#include <stdlib.h>

#define F_PI 3.14159265358979323846f


static Program program;

static TayGroup *particles_group;

static SphContext sph_context;

static int particles_count = 60000;

// static float sphere[10000];
// static unsigned sphere_subdivs = 2;

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

static void _update_sph_context(SphContext *c, float m) {
    c->m = m;

    float h2 = c->h * c->h;
    float h6 = h2 * h2 * h2;
    float h9 = h6 * h2 * c->h;

    c->h2 = h2;

    /* Matthias Müller, David Charypar and Markus Gross (2003) */
    c->poly6 = m * 315.0f / (64.0f * F_PI * h9);
    c->poly6_gradient = m * -945.0f / (32.0f * F_PI * h9);
    c->poly6_laplacian = c->poly6_gradient;
    c->spiky = m * -45.0f / (F_PI * h6);
    c->viscosity = -c->spiky;
}

void fluid_init() {

    // icosahedron_verts(sphere_subdivs, sphere);

    float particle_m = 0.05f;
    float fluid_density = 998.29f;
    float atmospheric_pressure = 101325.0f;
    float total_m = particles_count * particle_m;
    float initial_volume = total_m / fluid_density;

    int particles_inside_influence_radius = 20;

    sph_context.K = atmospheric_pressure / fluid_density; // from pV=mRT
    sph_context.density = fluid_density;
    sph_context.h = cbrtf(3.0f * (particles_inside_influence_radius * (initial_volume / particles_count)) / (4.0f * F_PI));
    sph_context.dt = 0.005f;
    sph_context.dynamic_viscosity = 3.5f;
    sph_context.surface_tension = 0.0728f;
    sph_context.surface_tension_threshold = 7.065f;
    sph_context.min = (float3){-1.0f, -1.8f, -1.0f};
    sph_context.max = (float3){1.0f, 1.8f, 1.0f};

    _update_sph_context(&sph_context, particle_m);

    float h = sph_context.h;
    float part_size = h * 1.0f;

    particles_group = tay_add_group(global.tay, sizeof(SphParticle), particles_count, TAY_TRUE);
    tay_configure_space(global.tay, particles_group, TAY_CPU_GRID, 3, (float4){part_size, part_size, part_size, part_size}, 250);

    tay_add_see(global.tay, particles_group, particles_group, sph_particle_density, (float4){h, h, h, h}, TAY_TRUE, &sph_context);
    tay_add_act(global.tay, particles_group, sph_particle_pressure, &sph_context);
    tay_add_see(global.tay, particles_group, particles_group, sph_force_terms, (float4){h, h, h, h}, TAY_FALSE, &sph_context);
    tay_add_act(global.tay, particles_group, sph_particle_leapfrog, &sph_context);

    for (int i = 0; i < particles_count; ++i) {
        SphParticle *p = tay_get_available_agent(global.tay, particles_group);
        p->p.x = _rand(sph_context.min.x, sph_context.min.x + (sph_context.max.x - sph_context.min.x) * 0.5f);
        p->p.y = _rand(sph_context.min.y, sph_context.max.y);
        p->p.z = _rand(sph_context.min.z, sph_context.min.z + (sph_context.max.z - sph_context.min.z) * 0.5f);
        p->vh = (float3){0.0f, 0.0f, 0.0f};
        p->v = (float3){0.0f, 0.0f, 0.0f};
        sph_particle_reset(p);
        tay_commit_available_agent(global.tay, particles_group);
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
    mat4_translate(&modelview, 0.0f, 130.0f, -800.0f);
    mat4_rotate(&modelview, -1.2f, 1.0f, 0.0f, 0.0f);
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
        SphParticle *p = tay_get_agent(global.tay, particles_group, i);
        inst_pos[i].x = p->p.x * 200.0f;
        inst_pos[i].y = p->p.y * 200.0f;
        inst_pos[i].z = p->p.z * 200.0f;
        inst_size[i] = 1.0f;
        inst_energy[i] = 0.0f; // sqrtf(p->v.x * p->v.x + p->v.y * p->v.y + p->v.z * p->v.z) * 0.7f;
    }

    shader_program_set_data_float(&program, 1, particles_count, 3, inst_pos);
    shader_program_set_data_float(&program, 2, particles_count, 1, inst_size);
    shader_program_set_data_float(&program, 3, particles_count, 1, inst_energy);

    graphics_draw_triangles_instanced(CUBE_VERTS_COUNT, particles_count);
}
