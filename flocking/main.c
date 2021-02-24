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

static TayState *tay;
static int boids_group;
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
static vec3 *inst_pos;
static vec3 *inst_dir;
static float *inst_shd;
static int boids_count = 30000;
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

    double ms = tay_run(tay, 1, TAY_SPACE_CPU_GRID, 1);
    if ((++step % 50) == 0)
        printf("ms: %.4f\n", ms);
    tay_threads_report_telemetry(50);

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

    shader_program_use(&program);

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

    shader_program_set_uniform_mat4(&program, 0, &projection);

    shader_program_set_data_float(&program, 1, boids_count, 3, inst_pos);
    shader_program_set_data_float(&program, 2, boids_count, 3, inst_dir);
    shader_program_set_data_float(&program, 3, boids_count, 1, inst_shd);

    graphics_draw_triangles_instanced(18, boids_count);

    glfwSwapBuffers(window);
    // platform_sleep(10);
    glfwPollEvents();
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

    /* fill pyramid buffer */
    shader_program_set_data_float(&program, 0, 18, 3, pyramid);

    const float radius = 20.0f;
    const int max_boids_count = 100000;
    const float4 see_radii = { radius, radius, radius, radius };
    const float velocity = 1.0f;
    const float4 min = { -100.0f, -100.0f, -100.0f, -100.0f };
    const float4 max = { 100.0f, 100.0f, 100.0f, 100.0f };

    inst_pos = malloc(sizeof(vec3) * max_boids_count);
    inst_dir = malloc(sizeof(vec3) * max_boids_count);
    inst_shd = malloc(sizeof(float) * max_boids_count);

    ActContext act_context;
    SeeContext see_context;
    see_context.r = radius;
    see_context.separation_r = radius * 0.5f;

    tay_threads_start(); // TODO: remove this!!!

    tay = tay_create_state(3, see_radii);
    tay_set_source(tay, agent_kernels_source); // TODO: remove this!!!
    boids_group = tay_add_group(tay, sizeof(Agent), boids_count, 1);
    tay_add_see(tay, boids_group, boids_group, agent_see, "agent_see", see_radii, &see_context, sizeof(see_context));
    tay_add_act(tay, boids_group, agent_act, "agent_act", &act_context, sizeof(act_context));

    /* create agents and add them to tay */
    for (int i = 0; i < boids_count; ++i) {
        Agent *boid = tay_get_available_agent(tay, boids_group);
        boid->p.x = min.x + rand() * (max.x - min.x) / (float)RAND_MAX;
        boid->p.y = min.y + rand() * (max.y - min.y) / (float)RAND_MAX;
        boid->p.z = min.z + rand() * (max.z - min.z) / (float)RAND_MAX;
        boid->dir.x = -0.5f + rand() / (float)RAND_MAX;
        boid->dir.y = -0.5f + rand() / (float)RAND_MAX;
        boid->dir.z = -0.5f + rand() / (float)RAND_MAX;
        float l = velocity / sqrtf(boid->dir.x * boid->dir.x + boid->dir.y * boid->dir.y + boid->dir.z * boid->dir.z);
        boid->dir.x *= l;
        boid->dir.y *= l;
        boid->dir.z *= l;
        boid->speed = 1.0f;
        boid->separation = float3_null();
        boid->alignment = float3_null();
        boid->cohesion = float3_null();
        boid->cohesion_count = 0;
        boid->separation_count = 0;
        tay_commit_available_agent(tay, boids_group);

        if (i == camera) {
            camera_dir.x = boid->dir.x;
            camera_dir.y = boid->dir.y;
            camera_dir.z = boid->dir.z;
        }
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
