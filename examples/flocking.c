#include "main.h"
#include "tay.h"
#include "agent.h"
#include "shaders.h"
#include "graphics.h"
#include <math.h>
#include <stdlib.h>


static Program program;

static TayGroup *boids_group;

static ActContext act_context;
static SeeContext see_context;

static int boids_count = 80000;

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

void flocking_init() {
    TayState *tay = global.tay;

    const float radius = 20.0f;
    const float avoidance_r = 100.0f;
    const float4 see_radii = { radius, radius, radius, radius };
    const float4 avoidance_see_radii = { avoidance_r, avoidance_r, avoidance_r, avoidance_r };
    const float part_radius = radius * 0.5f;
    const float4 part_radii = { part_radius, part_radius, part_radius, part_radius };
    const float4 obstacle_part_radii = { 10.0f, 10.0f, 10.0f, 10.0f };
    const float velocity = 1.0f;
    const float4 min = { -300.0f, -300.0f, -300.0f, -300.0f };
    const float4 max = { 300.0f, 300.0f, 300.0f, 300.0f };

    see_context.r = radius;
    see_context.separation_r = radius * 0.5f;

    boids_group = tay_add_group(tay, sizeof(Agent), boids_count, TAY_TRUE);
    tay_configure_space(tay, boids_group, TAY_CPU_GRID, 3, part_radii, 250);

    tay_add_see(tay, boids_group, boids_group, agent_see, see_radii, TAY_FALSE, &see_context);
    tay_add_act(tay, boids_group, agent_act, &act_context);

    for (int i = 0; i < boids_count; ++i) {
        Agent *boid = tay_get_available_agent(tay, boids_group);
        boid->p.x = _rand(min.x, max.x);
        boid->p.y = _rand(min.y, max.y);
        boid->p.z = _rand(min.z, max.z);
        boid->dir.x = _rand(-1.0f, 1.0f);
        boid->dir.y = _rand(-1.0f, 1.0f);
        boid->dir.z = _rand(-1.0f, 1.0f);
        float l = velocity / sqrtf(boid->dir.x * boid->dir.x + boid->dir.y * boid->dir.y + boid->dir.z * boid->dir.z);
        boid->dir.x *= l;
        boid->dir.y *= l;
        boid->dir.z *= l;
        boid->speed = 1.0f;
        boid->separation = (float3){0.0f, 0.0f, 0.0f};
        boid->alignment = (float3){0.0f, 0.0f, 0.0f};
        boid->cohesion = (float3){0.0f, 0.0f, 0.0f};
        boid->cohesion_count = 0;
        boid->separation_count = 0;
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

void flocking_draw() {
    TayState *tay = global.tay;
    vec3 *inst_pos = global.inst_vec3_buffers[0];
    vec3 *inst_dir = global.inst_vec3_buffers[1];
    float *inst_shd = global.inst_float_buffers[0];

    mat4 perspective;
    graphics_perspective(&perspective, 1.2f, (float)global.window_w / (float)global.window_h, 1.0f, 4000.0f);

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
