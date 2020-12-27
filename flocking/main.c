#include "tay.h"
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

static void _close_callback(GLFWwindow *window) {
    window_quit = true;
}

static void _main_loop_func(GLFWwindow *window) {
    graphics_viewport(0, 0, window_w, window_h);
    graphics_clear(0.2f, 0.2f, 0.2f);
    graphics_clear_depth();
    graphics_enable_depth_test(1);

    glfwSwapBuffers(window);
    platform_sleep(10);
    glfwPollEvents();
}

int main() {
    const float4 see_radii = { 10.0f, 10.0f, 10.0f, 10.0f };
    const int boids_count = 10000;
    const float velocity = 1.0f;
    const float4 min = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float4 max = { 100.0f, 100.0f, 100.0f, 100.0f };

    ActContext act_context;
    SeeContext see_context;
    see_context.radii = see_radii;

    TayState *tay = tay_create_state(3, see_radii);
    int boids_group = tay_add_group(tay, sizeof(Agent), boids_count);
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

    while (!window_quit)
        _main_loop_func(window);

    tay_destroy_state(tay);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
