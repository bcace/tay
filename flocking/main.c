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
    -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 2.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 2.0f,
    1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 2.0f,
    -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 2.0f,
    -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
};
static vec3 *boids_draw_data;
static int boids_count = 10000;

static void _close_callback(GLFWwindow *window) {
    window_quit = true;
}

static void _main_loop_func(GLFWwindow *window) {
    graphics_viewport(0, 0, window_w, window_h);
    graphics_clear(0.2f, 0.2f, 0.2f);
    graphics_clear_depth();
    graphics_enable_depth_test(1);

    tay_run(tay, 1, TAY_SPACE_CPU_GRID, 0);

    // TODO: copy data to buffer

    shader_program_use(&program);

    mat4 perspective;
    graphics_perspective(&perspective, 0.8f, (float)window_w / (float)window_h, 1.0f, 1000.0f);
    vec3 pos = {0.0f, 0.0f, 500.0f};
    vec3 fwd = {0.0f, 0.0f, -1.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};

    mat4 lookat;
    graphics_lookat(&lookat, pos, fwd, up);

    mat4 projection;
    mat4_multiply(&projection, &perspective, &lookat);

    shader_program_set_uniform_mat4(&program, 0, &projection);

    mat4 modelview;
    mat4_set_identity(&modelview);
    shader_program_set_uniform_mat4(&program, 1, &modelview);

    for (int i = 0; i < boids_count; ++i) {
        Agent *boid = tay_get_agent(tay, boids_group, i);
        boids_draw_data[i].x = boid->p.x;
        boids_draw_data[i].y = boid->p.y;
        boids_draw_data[i].z = boid->p.z;
    }

    shader_program_set_data_float(&program, 1, boids_count, 3, boids_draw_data);
    graphics_draw_triangles_instanced(18, boids_count);

    glfwSwapBuffers(window);
    platform_sleep(10);
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
    shader_program_define_in_float_instanced(&program, 3); /* instance position */
    shader_program_define_uniform(&program, "projection");
    shader_program_define_uniform(&program, "modelview");

    /* fill pyramid buffer */
    shader_program_set_data_float(&program, 0, 18, 3, pyramid);

    const int max_boids_count = 100000;
    const float4 see_radii = { 10.0f, 10.0f, 10.0f, 10.0f };
    const float velocity = 1.0f;
    const float4 min = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float4 max = { 100.0f, 100.0f, 100.0f, 100.0f };

    boids_draw_data = malloc(sizeof(vec3) * max_boids_count);

    ActContext act_context;
    SeeContext see_context;
    see_context.radii = see_radii;

    tay_runner_init(); // TODO: remove this!!!
    tay_runner_start_threads(8); // TODO: remove this!!!

    tay = tay_create_state(3, see_radii);
    tay_set_source(tay, agent_kernels_source); // TODO: remove this!!!
    boids_group = tay_add_group(tay, sizeof(Agent), boids_count);
    tay_add_see(tay, boids_group, boids_group, agent_see, "agent_see", see_radii, &see_context, sizeof(see_context));
    tay_add_act(tay, boids_group, agent_act, "agent_act", &act_context, sizeof(act_context));

    /* create agents and add them to tay */
    for (int i = 0; i < boids_count; ++i) {
        Agent *boid = tay_get_available_agent(tay, boids_group);
        boid->p.x = min.x + rand() * (max.x - min.x) / (float)RAND_MAX;
        boid->p.y = min.y + rand() * (max.y - min.y) / (float)RAND_MAX;
        boid->p.z = min.z + rand() * (max.z - min.z) / (float)RAND_MAX;
        boid->v.x = -0.5f + rand() / (float)RAND_MAX;
        boid->v.y = -0.5f + rand() / (float)RAND_MAX;
        boid->v.z = -0.5f + rand() / (float)RAND_MAX;
        float l = velocity / sqrtf(boid->v.x * boid->v.x + boid->v.y * boid->v.y + boid->v.z * boid->v.z);
        boid->v.x *= l;
        boid->v.y *= l;
        boid->v.z *= l;
        tay_commit_available_agent(tay, boids_group);
    }

    tay_simulation_start(tay);

    while (!window_quit)
        _main_loop_func(window);

    tay_simulation_end(tay);
    tay_destroy_state(tay);

    tay_runner_stop_threads(); // TODO: remove this!!!

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
