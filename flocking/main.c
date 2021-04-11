#include "tay.h"
#include "thread.h" // TODO: remove this!!!
#include "agent.h"
#include "graphics.h"
#include "platform.h"
#include "shaders.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>


static int window_h = 800;
static int window_w = 1600;
static bool window_quit = false;
static Program program;
static Program obstacles_program;

static TayState *tay;
static TayGroup *boids_group;
static TayGroup *obstacles_group;

static float pyramid[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 4.0f,

    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 4.0f,

    1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 4.0f,

    -1.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 4.0f,

    -1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,

    -1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
};

static const int pyramid_verts_count = 18;

#define ICO_X 0.525731112119133696f
#define ICO_Z 0.850650808352039932f

#define V0  -ICO_X, 0.0f, ICO_Z
#define V1  ICO_X, 0.0f, ICO_Z
#define V2  -ICO_X, 0.0f, -ICO_Z
#define V3  ICO_X, 0.0f, -ICO_Z
#define V4  0.0f, ICO_Z, ICO_X
#define V5  0.0f, ICO_Z, -ICO_X
#define V6  0.0f, -ICO_Z, ICO_X
#define V7  0.0f, -ICO_Z, -ICO_X
#define V8  ICO_Z, ICO_X, 0.0f
#define V9  -ICO_Z, ICO_X, 0.0f
#define V10 ICO_Z, -ICO_X, 0.0f
#define V11 -ICO_Z, -ICO_X, 0.0f

static float icosahedron[] = {
    V1, V4, V0,
    V4, V9, V0,
    V4, V5, V9,
    V8, V5, V4,
    V1, V8, V4,
    V1, V10, V8,
    V10, V3, V8,
    V8, V3, V5,
    V3, V2, V5,
    V3, V7, V2,
    V3, V10, V7,
    V10, V6, V7,
    V6, V11, V7,
    V6, V0, V11,
    V6, V1, V0,
    V10, V1, V6,
    V11, V0, V9,
    V2, V11, V9,
    V5, V2, V9,
    V11, V2, V7,
};

static const int icosahedron_verts_count = 180;

static vec3 *inst_pos;
static vec3 *inst_dir;
static float *inst_shd;
static int boids_count = 10000;
static int obstacles_count = 20;
static int camera = -1;
static float3 camera_dir;

static void _close_callback(GLFWwindow *window) {
    window_quit = true;
}

static int step = 0;

static void _main_loop_func(GLFWwindow *window) {
    graphics_viewport(0, 0, window_w, window_h);
    graphics_clear(0.98f, 0.98f, 0.98f);
    graphics_clear_depth();
    graphics_enable_depth_test(1);

    tay_run(tay, 1);
    double ms = tay_get_ms_per_step_for_last_run(tay);
    if ((++step % 50) == 0)
        printf("ms: %.4f\n", ms);
    tay_threads_report_telemetry(50, 0);

    mat4 perspective;
    graphics_perspective(&perspective, 1.2f, (float)window_w / (float)window_h, 1.0f, 2000.0f);

    vec3 pos, fwd, up;
    if (camera >= 0 && camera < boids_count) {
        Agent *watch_boid = tay_get_agent(tay, boids_group, camera);

        camera_dir.x += watch_boid->dir.x * 0.005f;
        camera_dir.y += watch_boid->dir.y * 0.005f;
        camera_dir.z += watch_boid->dir.z * 0.005f;
        float l = sqrtf(camera_dir.x * camera_dir.x + camera_dir.y * camera_dir.y + camera_dir.z * camera_dir.z);
        camera_dir.x /= l;
        camera_dir.y /= l;
        camera_dir.z /= l;

        pos.x = watch_boid->p.x - camera_dir.x * 200.0f;
        pos.y = watch_boid->p.y - camera_dir.y * 200.0f;
        pos.z = watch_boid->p.z - camera_dir.z * 200.0f;
        fwd.x = camera_dir.x;
        fwd.y = camera_dir.y;
        fwd.z = camera_dir.z;
        up.x = 0.0f;
        up.y = 0.0f;
        up.z = 1.0f;
    }
    else {
        pos.x = -1000.0f;
        pos.y = 0.0f;
        pos.z = 0.0f;
        fwd.x = 1.0f;
        fwd.y = 0.0f;
        fwd.z = 0.0f;
        up.x = 0.0f;
        up.y = 0.0f;
        up.z = 1.0f;
    }

    mat4 lookat;
    graphics_lookat(&lookat, pos, fwd, up);

    mat4 projection;
    mat4_multiply(&projection, &perspective, &lookat);


    /* draw boids */
    {
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
            if (i == camera)
                inst_shd[i] = 1.0f;
            else
                inst_shd[i] = 0.0f;
        }

        shader_program_set_data_float(&program, 0, pyramid_verts_count, 3, pyramid);
        shader_program_set_data_float(&program, 1, boids_count, 3, inst_pos);
        shader_program_set_data_float(&program, 2, boids_count, 3, inst_dir);
        shader_program_set_data_float(&program, 3, boids_count, 1, inst_shd);

        graphics_draw_triangles_instanced(pyramid_verts_count, boids_count);
    }

    /* draw obstacles */
    {
        shader_program_use(&obstacles_program);
        shader_program_set_uniform_mat4(&obstacles_program, 0, &projection);

        for (int i = 0; i < obstacles_count; ++i) {
            Obstacle *agent = tay_get_agent(tay, obstacles_group, i);
            float radius = agent->radius;
            inst_pos[i].x = agent->min.x + radius;
            inst_pos[i].y = agent->min.y + radius;
            inst_pos[i].z = agent->min.z + radius;
            inst_shd[i] = radius;
        }

        shader_program_set_data_float(&obstacles_program, 0, icosahedron_verts_count, 3, icosahedron);
        shader_program_set_data_float(&obstacles_program, 1, obstacles_count, 3, inst_pos);
        shader_program_set_data_float(&obstacles_program, 2, obstacles_count, 1, inst_shd);

        graphics_draw_triangles_instanced(icosahedron_verts_count, obstacles_count);
    }

    glfwSwapBuffers(window);
    // platform_sleep(10);
    glfwPollEvents();
}

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

static float _rand_exponential(float min, float max, float exp) {
    float base = rand() / (float)RAND_MAX;
    return min + (max - min) * powf(base, exp);
}

int main() {

    if (!glfwInit()) {
        fprintf(stderr, "Could not initialize GLFW\n");
        return;
    }

    GLFWmonitor *monitor = 0;
    GLFWwindow *window = glfwCreateWindow(window_w, window_h, "Tay", monitor, 0);

    if (!window) {
        fprintf(stderr, "Could not create GLFW window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowCloseCallback(window, _close_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    shader_program_init(&program, boids_vert, "boids.vert", boids_frag, "boids.frag");
    shader_program_define_in_float(&program, 3); /* vertex position */
    shader_program_define_instanced_in_float(&program, 3); /* instance position */
    shader_program_define_instanced_in_float(&program, 3); /* instance direction */
    shader_program_define_instanced_in_float(&program, 1); /* instance shade */
    shader_program_define_uniform(&program, "projection");

    shader_program_init(&obstacles_program, obstacles_vert, "obstacles.vert", obstacles_frag, "boids.frag");
    shader_program_define_in_float(&obstacles_program, 3); /* vertex position */
    shader_program_define_instanced_in_float(&obstacles_program, 3); /* instance position */
    shader_program_define_instanced_in_float(&obstacles_program, 1); /* instance size */
    shader_program_define_uniform(&obstacles_program, "projection");

    const float radius = 20.0f;
    const float avoidance_r = 100.0f;
    const int max_agents_count = 100000;
    const float4 see_radii = { radius, radius, radius, radius };
    const float4 avoidance_see_radii = { avoidance_r, avoidance_r, avoidance_r, avoidance_r };
    const float part_radius = radius * 0.5f;
    const float4 part_radii = { part_radius, part_radius, part_radius, part_radius };
    const float4 obstacle_part_radii = { 10.0f, 10.0f, 10.0f, 10.0f };
    const float velocity = 1.0f;
    const float4 min = { -300.0f, -300.0f, -300.0f, -300.0f };
    const float4 max = { 300.0f, 300.0f, 300.0f, 300.0f };

    inst_pos = malloc(sizeof(vec3) * max_agents_count);
    inst_dir = malloc(sizeof(vec3) * max_agents_count);
    inst_shd = malloc(sizeof(float) * max_agents_count);

    ActContext act_context;
    SeeContext see_context;
    see_context.r = radius;
    see_context.separation_r = radius * 0.5f;
    see_context.avoidance_r = avoidance_r;
    see_context.avoidance_c = 0.001f;

    tay_threads_start(); // TODO: remove this!!!

    tay = tay_create_state();

    boids_group = tay_add_group(tay, sizeof(Agent), boids_count, TAY_TRUE);
    tay_configure_space(tay, boids_group, TAY_CPU_GRID, 3, part_radii, 250);
    obstacles_group = tay_add_group(tay, sizeof(Obstacle), obstacles_count, TAY_FALSE);
    tay_configure_space(tay, obstacles_group, TAY_CPU_AABB_TREE, 3, obstacle_part_radii, 100);

    tay_add_see(tay, boids_group, boids_group, agent_see, see_radii, &see_context);
    tay_add_see(tay, boids_group, obstacles_group, agent_obstacle_see, avoidance_see_radii, &see_context);
    tay_add_act(tay, boids_group, agent_act, &act_context);

    /* boids */
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
        boid->separation = (float3){ 0.0f, 0.0f, 0.0f };
        boid->alignment = (float3){ 0.0f, 0.0f, 0.0f };
        boid->cohesion = (float3){ 0.0f, 0.0f, 0.0f };
        boid->avoidance = (float3){ 0.0f, 0.0f, 0.0f };
        boid->cohesion_count = 0;
        boid->separation_count = 0;
        tay_commit_available_agent(tay, boids_group);

        if (i == camera) {
            camera_dir.x = boid->dir.x;
            camera_dir.y = boid->dir.y;
            camera_dir.z = boid->dir.z;
        }
    }

    /* obstacles */
    for (int i = 0; i < obstacles_count; ++i) {
        Obstacle *agent = tay_get_available_agent(tay, obstacles_group);
        float radius = _rand_exponential(80.0f, 80.0f, 20.0f);
        float diameter = radius * 2.0f;
        agent->min.x = _rand(min.x, max.x - diameter);
        agent->min.y = _rand(min.y, max.y - diameter);
        agent->min.z = _rand(min.z, max.z - diameter);
        agent->max.x = agent->min.x + diameter;
        agent->max.y = agent->min.y + diameter;
        agent->max.z = agent->min.z + diameter;
        agent->radius = radius;
        tay_commit_available_agent(tay, obstacles_group);
    }

    tay_simulation_start(tay);

    while (!window_quit)
        _main_loop_func(window);

    tay_simulation_end(tay);
    tay_destroy_state(tay);

    tay_threads_stop(); // TODO: remove this!!!

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
