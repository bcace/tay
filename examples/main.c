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


extern Global demos;

static void _close_callback(GLFWwindow *window) {
    demos.window_quit = true;
}

static void _main_loop_func(GLFWwindow *window) {
    static int step = 0;

    graphics_viewport(0, 0, demos.window_w, demos.window_h);
    graphics_clear(0.98f, 0.98f, 0.98f);
    graphics_clear_depth();
    graphics_enable_depth_test(1);

    tay_run(demos.tay, 1);
    double ms = tay_get_ms_per_step_for_last_run(demos.tay);
    if ((++step % 1) == 0)
        printf("ms: %.4f\n", ms);
    tay_threads_report_telemetry(50, 0);

    /* drawing */
    if (demos.example == FLOCKING)
        flocking_draw();
    else if (demos.example == PIC_FLOCKING)
        pic_flocking_draw();
    else if (demos.example == FLUID)
        fluid_draw();

    glfwSwapBuffers(window);
    // platform_sleep(10);
    glfwPollEvents();
}

int main() {
    demos.example = PIC_FLOCKING;

    if (!glfwInit()) {
        fprintf(stderr, "Could not initialize GLFW\n");
        return;
    }

    demos.window_quit = false;
    demos.window_h = 800;
    demos.window_w = 1600;
    demos.max_agents_count = 1000000;

    GLFWmonitor *monitor = 0;
    GLFWwindow *window = glfwCreateWindow(demos.window_w, demos.window_h, "Tay", monitor, 0);

    if (!window) {
        fprintf(stderr, "Could not create GLFW window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowCloseCallback(window, _close_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    /* initialize draw buffers */
    demos.inst_vec3_buffers[0] = malloc(sizeof(vec3) * demos.max_agents_count);
    demos.inst_vec3_buffers[1] = malloc(sizeof(vec3) * demos.max_agents_count);
    demos.inst_float_buffers[0] = malloc(sizeof(float) * demos.max_agents_count);
    demos.inst_float_buffers[1] = malloc(sizeof(float) * demos.max_agents_count);

    tay_threads_start(100000); // TODO: remove this!!!
    demos.tay = tay_create_state();

    if (demos.example == FLOCKING)
        flocking_init();
    else if (demos.example == PIC_FLOCKING)
        pic_flocking_init();
    else if (demos.example == FLUID)
        fluid_init();

    // platform_sleep(5000);

    tay_simulation_start(demos.tay);

    while (!demos.window_quit)
        _main_loop_func(window);

    tay_simulation_end(demos.tay);
    tay_destroy_state(demos.tay);

    tay_threads_stop(); // TODO: remove this!!!

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
