#include "main.h"
#include "tay.h"
#include "thread.h" // TODO: remove this!!!
#include "graphics.h"
#include "platform.h"
#include "shaders.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>


extern Global global;

static void _close_callback(GLFWwindow *window) {
    global.window_quit = true;
}

static void _main_loop_func(GLFWwindow *window) {
    static int step = 0;

    graphics_viewport(0, 0, global.window_w, global.window_h);
    graphics_clear(0.98f, 0.98f, 0.98f);
    graphics_clear_depth();
    graphics_enable_depth_test(1);

    tay_run(global.tay, 1);
    double ms = tay_get_ms_per_step_for_last_run(global.tay);
    if ((++step % 50) == 0)
        printf("ms: %.4f\n", ms);
    tay_threads_report_telemetry(50, 0);

    /* drawing */
    if (global.example == FLOCKING)
        flocking_draw();
    else if (global.example == FLUID)
        fluid_draw();

    glfwSwapBuffers(window);
    // platform_sleep(10);
    glfwPollEvents();
}

int main() {
    global.example = FLUID;

    if (!glfwInit()) {
        fprintf(stderr, "Could not initialize GLFW\n");
        return;
    }

    global.window_quit = false;
    global.window_h = 800;
    global.window_w = 1600;
    global.max_agents_count = 200000;

    GLFWmonitor *monitor = 0;
    GLFWwindow *window = glfwCreateWindow(global.window_w, global.window_h, "Tay", monitor, 0);

    if (!window) {
        fprintf(stderr, "Could not create GLFW window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowCloseCallback(window, _close_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    /* initialize draw buffers */
    global.inst_vec3_buffers[0] = malloc(sizeof(vec3) * global.max_agents_count);
    global.inst_vec3_buffers[1] = malloc(sizeof(vec3) * global.max_agents_count);
    global.inst_float_buffers[0] = malloc(sizeof(float) * global.max_agents_count);
    global.inst_float_buffers[1] = malloc(sizeof(float) * global.max_agents_count);

    tay_threads_start(100000); // TODO: remove this!!!
    global.tay = tay_create_state();

    if (global.example == FLOCKING)
        flocking_init();
    else if (global.example == FLUID)
        fluid_init();

    platform_sleep(5000);

    tay_simulation_start(global.tay);

    while (!global.window_quit)
        _main_loop_func(window);

    tay_simulation_end(global.tay);
    tay_destroy_state(global.tay);

    tay_threads_stop(); // TODO: remove this!!!

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
