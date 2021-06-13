#include "main.h"
#include "tay.h"
#include "shaders.h"
#include "graphics.h"
#include "agent_host.h"
#include <stdlib.h>
#include <math.h>


static int boids_count = 40000;
static float pic_cell_size = 20.0f;
static float pic_transfer_r = 40.0f;

static TayGroup *boids_group;
static TayPicGrid *pic;

static PicFlockingContext context;

static Program program;


static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

void pic_flocking_init() {
    TayState *tay = demos.tay;

    boids_group = tay_add_group(tay, sizeof(PicBoid), boids_count, TAY_TRUE);
    pic = tay_add_pic_grid(tay, sizeof(PicBoidNode), 1000000, pic_cell_size);

    context.radius = pic_transfer_r;
    context.min = (float4){-500.0f, -500.0f, -500.0f, -500.0f};
    context.max = (float4){ 500.0f,  500.0f,  500.0f,  500.0f};

    tay_add_pic_act(tay, pic, pic_reset_node, 0);
    tay_add_pic_see(tay, boids_group, pic, pic_transfer_boid_to_node, (float4){pic_transfer_r, pic_transfer_r, pic_transfer_r, pic_transfer_r}, &context);
    tay_add_pic_see(tay, boids_group, pic, pic_transfer_node_to_boids, (float4){pic_transfer_r, pic_transfer_r, pic_transfer_r, pic_transfer_r}, &context);
    tay_add_act(tay, boids_group, pic_boid_action, "pic_boid_action", &context, sizeof(context));

    for (int i = 0; i < boids_count; ++i) {
        PicBoid *boid = tay_get_available_agent(tay, boids_group);
        boid->p.x = _rand(context.min.x, context.max.x);
        boid->p.y = _rand(context.min.y, context.max.y);
        boid->p.z = _rand(context.min.z, context.max.z);
        boid->dir.x = _rand(-1.0f, 1.0f);
        boid->dir.y = _rand(-1.0f, 1.0f);
        boid->dir.z = _rand(-1.0f, 1.0f);
        float l = 1.0f / sqrtf(boid->dir.x * boid->dir.x + boid->dir.y * boid->dir.y + boid->dir.z * boid->dir.z);
        boid->dir.x *= l;
        boid->dir.y *= l;
        boid->dir.z *= l;
        boid->sep_f = (float4){0.0f, 0.0f, 0.0f, 0.0f};
        boid->dir_sum = (float4){0.0f, 0.0f, 0.0f, 0.0f};
        boid->coh_p = (float4){0.0f, 0.0f, 0.0f, 0.0f};
        boid->coh_count = 0;
        tay_commit_available_agent(tay, boids_group);
    }

    /* drawing init */

    shader_program_init(&program, boids_vert, "boids.vert", boids_frag, "boids.frag");
    shader_program_define_in_float(&program, 3); /* vertex position */
    shader_program_define_instanced_in_float(&program, 3); /* instance position */
    shader_program_define_instanced_in_float(&program, 3); /* instance direction */
    shader_program_define_instanced_in_float(&program, 1); /* instance shade */
    shader_program_define_uniform(&program, "projection");
}

void pic_flocking_draw() {
    TayState *tay = demos.tay;
    vec3 *inst_pos = demos.inst_vec3_buffers[0];
    vec3 *inst_dir = demos.inst_vec3_buffers[1];
    float *inst_shd = demos.inst_float_buffers[0];

    mat4 perspective;
    graphics_perspective(&perspective, 1.2f, (float)demos.window_w / (float)demos.window_h, 1.0f, 4000.0f);

    vec3 pos, fwd, up;
    pos.x = -2000.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    fwd.x = 1.0f;
    fwd.y = 0.0f;
    fwd.z = 0.0f;
    up.x = 0.0f;
    up.y = 0.0f;
    up.z = 1.0f;

    mat4 lookat;
    graphics_lookat(&lookat, pos, fwd, up);

    mat4 projection;
    mat4_multiply(&projection, &perspective, &lookat);

    shader_program_use(&program);
    shader_program_set_uniform_mat4(&program, 0, &projection);

    for (int i = 0; i < boids_count; ++i) {
        Agent *boid = tay_get_agent(tay, boids_group, i);
        inst_pos[i].x = boid->p.x;
        inst_pos[i].y = boid->p.y;
        inst_pos[i].z = boid->p.z;
        inst_dir[i].x = boid->dir.x;
        inst_dir[i].y = boid->dir.y;
        inst_dir[i].z = boid->dir.z;
        inst_shd[i] = 0.0f;
    }

    shader_program_set_data_float(&program, 0, PYRAMID_VERTS_COUNT, 3, PYRAMID_VERTS);
    shader_program_set_data_float(&program, 1, boids_count, 3, inst_pos);
    shader_program_set_data_float(&program, 2, boids_count, 3, inst_dir);
    shader_program_set_data_float(&program, 3, boids_count, 1, inst_shd);

    graphics_draw_triangles_instanced(PYRAMID_VERTS_COUNT, boids_count);
}
