#include "main.h"
#include "tay.h"
#include "agent.h"
#include "shaders.h"
#include "graphics.h"
#include <math.h>
#include <stdlib.h>


static Program program;

static TayGroup *particles_group;

static ParticleSeeContext particle_see_context;
static ParticleActContext particle_act_context;

static int particles_count = 30000;

static float icosahedron[] = {
    0.5257f, 0.0f, 0.8506f, 0.0f, 0.8506f, 0.5257f, -0.5257f, 0.0f, 0.8506f,
    0.0f, 0.8506f, 0.5257f, -0.8506f, 0.5257f, 0.0f, -0.5257f, 0.0f, 0.8506f,
    0.0f, 0.8506f, 0.5257f, 0.0f, 0.8506f, -0.5257f, -0.8506f, 0.5257f, 0.0f,
    0.8506f, 0.5257f, 0.0f, 0.0f, 0.8506f, -0.5257f, 0.0f, 0.8506f, 0.5257f,
    0.5257f, 0.0f, 0.8506f, 0.8506f, 0.5257f, 0.0f, 0.0f, 0.8506f, 0.5257f,
    0.5257f, 0.0f, 0.8506f, 0.8506f, -0.5257f, 0.0f, 0.8506f, 0.5257f, 0.0f,
    0.8506f, -0.5257f, 0.0f, 0.5257f, 0.0f, -0.8506f, 0.8506f, 0.5257f, 0.0f,
    0.8506f, 0.5257f, 0.0f, 0.5257f, 0.0f, -0.8506f, 0.0f, 0.8506f, -0.5257f,
    0.5257f, 0.0f, -0.8506f, -0.5257f, 0.0f, -0.8506f, 0.0f, 0.8506f, -0.5257f,
    0.5257f, 0.0f, -0.8506f, 0.0f, -0.8506f, -0.5257f, -0.5257f, 0.0f, -0.8506f,
    0.5257f, 0.0f, -0.8506f, 0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, -0.5257f,
    0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, 0.5257f, 0.0f, -0.8506f, -0.5257f,
    0.0f, -0.8506f, 0.5257f, -0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, -0.5257f,
    0.0f, -0.8506f, 0.5257f, -0.5257f, 0.0f, 0.8506f, -0.8506f, -0.5257f, 0.0f,
    0.0f, -0.8506f, 0.5257f, 0.5257f, 0.0f, 0.8506f, -0.5257f, 0.0f, 0.8506f,
    0.8506f, -0.5257f, 0.0f, 0.5257f, 0.0f, 0.8506f, 0.0f, -0.8506f, 0.5257f,
    -0.8506f, -0.5257f, 0.0f, -0.5257f, 0.0f, 0.8506f, -0.8506f, 0.5257f, 0.0f,
    -0.5257f, 0.0f, -0.8506f, -0.8506f, -0.5257f, 0.0f, -0.8506f, 0.5257f, 0.0f,
    0.0f, 0.8506f, -0.5257f, -0.5257f, 0.0f, -0.8506f, -0.8506f, 0.5257f, 0.0f,
    -0.8506f, -0.5257f, 0.0f, -0.5257f, 0.0f, -0.8506f, 0.0f, -0.8506f, -0.5257f,
};

static const int icosahedron_verts_count = 180;

static float cube[] = {
    -1, -1, -1, 1, -1, -1, -1, -1, 1,
    1, -1, -1, 1, -1, 1, -1, -1, 1,

    1, -1, -1, 1, 1, -1, 1, -1, 1,
    1, 1, -1, 1, 1, 1, 1, -1, 1,

    1, 1, -1, -1, 1, -1, 1, 1, 1,
    -1, 1, -1, -1, 1, 1, 1, 1, 1,

    -1, 1, -1, -1, -1, -1, -1, 1, 1,
    -1, -1, -1, -1, -1, 1, -1, 1, 1,

    -1, -1, -1, -1, 1, -1, 1, 1, -1,
    -1, -1, -1, 1, 1, -1, 1, -1, -1,

    -1, -1, 1, 1, -1, 1, 1, 1, 1,
    -1, -1, 1, 1, 1, 1, -1, 1, 1,
};

static int cube_verts_count = 36;

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

void fluid_init() {

    const float r = 10.0f;
    const float part_r = r * 0.5f;

    particles_group = tay_add_group(global.tay, sizeof(Particle), particles_count, TAY_TRUE);
    tay_configure_space(global.tay, particles_group, TAY_CPU_GRID, 3, (float4){part_r, part_r, part_r, part_r}, 250);

    particle_see_context.r = r;

    particle_act_context.min.x = -200.0f;
    particle_act_context.min.y = -40.0f;
    particle_act_context.min.z = -50.0f;
    particle_act_context.max.x = 200.0f;
    particle_act_context.max.y = 40.0f;
    particle_act_context.max.z = 300.0f;

    tay_add_see(global.tay, particles_group, particles_group, particle_see, (float4){r, r, r, r}, &particle_see_context);
    tay_add_act(global.tay, particles_group, particle_act, &particle_act_context);

    for (int i = 0; i < particles_count; ++i) {
        Particle *p = tay_get_available_agent(global.tay, particles_group);
        p->p.x = _rand(-200.0f, 0.0f);
        p->p.y = _rand(particle_act_context.min.y, particle_act_context.max.y);
        p->p.z = _rand(100.0f, 200.0f);
        p->v = (float3){0.0f, 0.0f, 0.0f};
        p->f = (float3){0.0f, 0.0f, 0.0f};
        tay_commit_available_agent(global.tay, particles_group);
    }

    /* drawing init */

    shader_program_init(&program, particles_vert, "particles.vert", particles_frag, "particles.frag");
    shader_program_define_in_float(&program, 3); /* vertex position */
    shader_program_define_instanced_in_float(&program, 3); /* instance position */
    shader_program_define_instanced_in_float(&program, 1); /* instance size */
    shader_program_define_uniform(&program, "projection");
}

void fluid_draw() {
    mat4 perspective;
    graphics_perspective(&perspective, 1.2f, (float)global.window_w / (float)global.window_h, 1.0f, 2000.0f);

    mat4 modelview;
    mat4_set_identity(&modelview);
    mat4_translate(&modelview, 0.0f, 0.0f, -500.0f);
    mat4_rotate(&modelview, -1.2f, 1.0f, 0.0f, 0.0f);
    mat4_rotate(&modelview, 0.7f, 0.0f, 0.0f, 1.0f);

    mat4 projection;
    mat4_multiply(&projection, &perspective, &modelview);

    shader_program_use(&program);
    shader_program_set_uniform_mat4(&program, 0, &projection);

    vec3 *inst_pos = global.inst_vec3_buffers[0];
    float *inst_size = global.inst_float_buffers[0];

    for (int i = 0; i < particles_count; ++i) {
        Particle *p = tay_get_agent(global.tay, particles_group, i);
        inst_pos[i].x = p->p.x;
        inst_pos[i].y = p->p.y;
        inst_pos[i].z = p->p.z;
        inst_size[i] = 0.8f;
    }

    shader_program_set_data_float(&program, 0, cube_verts_count, 3, cube);
    shader_program_set_data_float(&program, 1, particles_count, 3, inst_pos);
    shader_program_set_data_float(&program, 2, particles_count, 1, inst_size);

    graphics_draw_triangles_instanced(cube_verts_count, particles_count);
}
