#include "main.h"
#include "tay.h"
#include "agent_host.h"
#include "graphics.h"
#include "shaders.h"
#include <stdlib.h>
#include <math.h>


static int particles_count = 3000;
static float grid_resolution = 80; /* cells per side */

static TayGroup *group;
static TayPicGrid *pic;

static Taichi2DContext context;

static Program program;


static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

static void _add_object(TayState *tay, float min_x, float min_y, float max_x, float max_y, int count, int color) {
    for (int i = 0; i < count; ++i) {
        Taichi2DParticle *p = tay_get_available_agent(tay, group);
        p->p.x = _rand(min_x, max_x);
        p->p.y = _rand(min_y, max_y);
        p->color = color;
        taichi_2D_init_particle(p);
        tay_commit_available_agent(tay, group);
    }
}

void taichi_2D_init() {
    TayState *tay = demos.tay;

    group = tay_add_group(tay, sizeof(Taichi2DParticle), particles_count, TAY_TRUE);
    pic = tay_add_pic_grid(tay, sizeof(Taichi2DNode), 10000000, 1.0f / grid_resolution, 2);

    tay_configure_space(tay, group, TAY_CPU_SIMPLE, 2, (float4){0.0f}, 0);
    tay_fix_space_box(tay, group, (float4){0.0f, 0.0f, 0.0f, 0.0f}, (float4){1.0f, 1.0f, 1.0f, 1.0f});

    const float r = 0.08f;
    _add_object(tay, 0.55f - r, 0.45f - r, 0.55f + r, 0.45f + r, particles_count / 3, 0);
    _add_object(tay, 0.45f - r, 0.65f - r, 0.45f + r, 0.65f + r, particles_count / 3, 1);
    _add_object(tay, 0.55f - r, 0.85f - r, 0.55f + r, 0.85f + r, particles_count / 3, 2);

    context.dt = 1e-4f;
    context.plastic = 1;

    tay_add_pic_act(tay, pic, taichi_2D_reset_node, &context);
    tay_add_pic_see(tay, group, pic, taichi_2D_particle_to_node, 3, &context);
    tay_add_pic_act(tay, pic, taichi_2D_node, &context);
    tay_add_pic_see(tay, group, pic, taichi_2D_node_to_particle, 3, &context);

    /* drawing init */

    shader_program_init(&program, taichi_vert, "taichi.vert", taichi_frag, "taichi.frag");
    shader_program_define_in_float(&program, 3); /* vertex position */
    shader_program_define_instanced_in_float(&program, 3); /* instance position */
    shader_program_define_instanced_in_float(&program, 1); /* instance color */
    shader_program_define_uniform(&program, "projection");
}

void taichi_2D_draw() {
    TayState *tay = demos.tay;
    vec3 *inst_pos = demos.inst_vec3_buffers[0];
    float *inst_col = demos.inst_float_buffers[0];

    mat4 perspective;
    graphics_perspective(&perspective, 1.2f, (float)demos.window_w / (float)demos.window_h, 1.0f, 400.0f);

    vec3 pos, fwd, up;
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 2.0f;
    fwd.x = 0.0f;
    fwd.y = 0.0f;
    fwd.z = -1.0f;
    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;

    mat4 lookat;
    graphics_lookat(&lookat, pos, fwd, up);

    mat4 projection;
    mat4_multiply(&projection, &perspective, &lookat);

    shader_program_use(&program);
    shader_program_set_uniform_mat4(&program, 0, &projection);

    for (int i = 0; i < particles_count; ++i) {
        Taichi2DParticle *p = tay_get_agent(tay, group, i);
        inst_pos[i].x = p->p.x;
        inst_pos[i].y = p->p.y;
        inst_pos[i].z = 0.0f;
        inst_col[i] = (float)p->color;
    }

    shader_program_set_data_float(&program, 0, CUBE_VERTS_COUNT, 3, CUBE_VERTS);
    shader_program_set_data_float(&program, 1, particles_count, 3, inst_pos);
    shader_program_set_data_float(&program, 2, particles_count, 1, inst_col);

    graphics_draw_triangles_instanced(CUBE_VERTS_COUNT, particles_count);
}
