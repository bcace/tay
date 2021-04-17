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
static TayGroup *balls_group;

static SphContext sph_context;

static int particles_count = 20000;
static int balls_count = 2;
static float ball_r = 50.0f;

static float sphere[10000];
static unsigned sphere_subdivs = 2;

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

static void _init_sph_context(SphContext *c, float h, float k, float mu, float rho0, float dt, float3 min, float3 max) {
    c->h = h;
    c->k = k;
    c->mu = mu;
    c->rho0 = rho0;
    c->dt = dt;
    c->min = min;
    c->max = max;
}

static void _update_sph_context(SphContext *c, float m) {
    c->m = m;
    c->h2 = c->h * c->h;

    /* density: poly6, Matthias Müller, David Charypar and Markus Gross (2003) */
    c->poly6 = 315.0f * m / (64.0f * F_PI * c->h2 * c->h2 * c->h2 * c->h2 * c->h);
    /* pressure: spiky */
    c->spiky = -45.0f * m / (F_PI * c->h2 * c->h2 * c->h2);

    /* acceleration: Bindel, Fall (2011) */
    c->C0 = m / (F_PI * c->h2 * c->h2);
    c->Cp = 15.0f * c->k;
    c->Cv = -40.0f * c->mu;
}

void fluid_init() {
    const float h = 0.05f;
    const float part_r = h * 0.5f;

    icosahedron_verts(sphere_subdivs, sphere);

    particles_group = tay_add_group(global.tay, sizeof(SphParticle), particles_count, TAY_TRUE);
    tay_configure_space(global.tay, particles_group, TAY_CPU_GRID, 3, (float4){part_r, part_r, part_r, part_r}, 250);

    float particle_m = 1.0f;

    _init_sph_context(&sph_context,
        h, 1000.0f, 0.1f, 1000.0f, 0.001f,
        (float3){-1.0f, -1.0f, -1.0f},
        (float3){1.0f, 1.0f, 1.0f}
    );
    _update_sph_context(&sph_context, particle_m);

    tay_add_see(global.tay, particles_group, particles_group, sph_particle_density, (float4){h, h, h, h}, TAY_TRUE, &sph_context);
    tay_add_see(global.tay, particles_group, particles_group, sph_particle_acceleration, (float4){h, h, h, h}, TAY_TRUE, &sph_context);
    tay_add_act(global.tay, particles_group, sph_particle_leapfrog, &sph_context);

    for (int i = 0; i < particles_count; ++i) {
        SphParticle *p = tay_get_available_agent(global.tay, particles_group);
        p->p.x = _rand(sph_context.min.x, sph_context.max.x);
        p->p.y = _rand(sph_context.min.y, sph_context.max.y);
        p->p.z = _rand(sph_context.min.z, sph_context.max.z);
        p->pressure_accum = (float3){0.0f, 0.0f, 0.0f};
        p->viscosity_accum = (float3){0.0f, 0.0f, 0.0f};
        p->vh = (float3){0.0f, 0.0f, 0.0f};
        p->v = (float3){0.0f, 0.0f, 0.0f};
        sph_particle_reset(p);
        tay_commit_available_agent(global.tay, particles_group);
    }

    // assume mass to get densities, and then scale particle mass so we achieve reference density

    {
        for (int a_i = 0; a_i < particles_count; ++a_i) {
            SphParticle *a = tay_get_agent(global.tay, particles_group, a_i);

            for (int b_i = 0; b_i < particles_count; ++b_i) {

                if (b_i == a_i)
                    continue;

                SphParticle *b = tay_get_agent(global.tay, particles_group, b_i);

                sph_particle_density(a, b, &sph_context); // TODO: execute this as a standalone pass execution, with threading and all (maybe within simulation, as a pre-step)
            }
        }

        float rho2s = 0.0f;
        float rhos = 0.0f;

        for (int a_i = 0; a_i < particles_count; ++a_i) {
            SphParticle *a = tay_get_agent(global.tay, particles_group, a_i);
            rhos += a->density;
            rho2s += a->density * a->density;

            sph_particle_reset(a);
        }

        particle_m *= sph_context.rho0 * rhos / rho2s;
    }

    _update_sph_context(&sph_context, particle_m);

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
        SphParticle *p = tay_get_agent(global.tay, particles_group, i);
        inst_pos[i].x = p->p.x * 200.0f;
        inst_pos[i].y = p->p.y * 200.0f;
        inst_pos[i].z = p->p.z * 200.0f;
        inst_size[i] = 1.0f;
        inst_energy[i] = 0.0f;// sqrtf(p->v.x * p->v.x + p->v.y * p->v.y + p->v.z * p->v.z) * 0.1f;
    }

    shader_program_set_data_float(&program, 1, particles_count, 3, inst_pos);
    shader_program_set_data_float(&program, 2, particles_count, 1, inst_size);
    shader_program_set_data_float(&program, 3, particles_count, 1, inst_energy);

    graphics_draw_triangles_instanced(CUBE_VERTS_COUNT, particles_count);

    // shader_program_set_data_float(&program, 0, icosahedron_verts_count(sphere_subdivs), 3, sphere);

    // for (int i = 0; i < balls_count; ++i) {
    //     Ball *b = tay_get_agent(global.tay, balls_group, i);
    //     inst_pos[i].x = (b->min.x + b->max.x) * 0.5f;
    //     inst_pos[i].y = (b->min.y + b->max.y) * 0.5f;
    //     inst_pos[i].z = (b->min.z + b->max.z) * 0.5f;
    //     inst_size[i] = ball_r;
    //     inst_energy[i] = 0.0f;
    // }

    // shader_program_set_data_float(&program, 1, balls_count, 3, inst_pos);
    // shader_program_set_data_float(&program, 2, balls_count, 1, inst_size);
    // shader_program_set_data_float(&program, 3, balls_count, 1, inst_energy);

    // graphics_draw_triangles_instanced(icosahedron_verts_count(sphere_subdivs), balls_count);
}
