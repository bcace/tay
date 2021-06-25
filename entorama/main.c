#include "main.h"
#include "entorama.h"
#include "shaders.h"
#include "tay.h"
#include "thread.h" // TODO: remove this!!!
#include "graphics.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>


static int quit = 0;
static int window_w = 1600;
static int window_h = 800;

static void _close_callback(GLFWwindow *window) {
    quit = 1;
}

int main() {

    if (!glfwInit()) {
        fprintf(stderr, "Could not initialize GLFW\n");
        return 1;
    }

    GLFWmonitor *monitor = 0;
    GLFWwindow *window = glfwCreateWindow(1600, 800, "Entorama", monitor, 0);

    if (!window) {
        fprintf(stderr, "Could not create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowCloseCallback(window, _close_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    Program program;

    shader_program_init(&program, boids_vert, "boids.vert", boids_frag, "boids.frag");
    shader_program_define_in_float(&program, 3); /* vertex position */
    shader_program_define_instanced_in_float(&program, 3); /* instance position */
    shader_program_define_instanced_in_float(&program, 3); /* instance direction */
    shader_program_define_instanced_in_float(&program, 1); /* instance shade */
    shader_program_define_uniform(&program, "projection");

    EntoramaModelInfo model_info;
    model_load(&model_info, "m_flocking.dll");

    TayState *tay = tay_create_state();

    model_info.init(tay);

    tay_threads_start(100000); // TODO: remove this!!!
    tay_simulation_start(tay);

    while (!quit) {
        graphics_viewport(0, 0, window_w, window_h);
        vec4 bg = color_bg();
        graphics_clear(bg.x, bg.y, bg.z);
        graphics_clear_depth();
        graphics_enable_depth_test(1);

        tay_run(tay, 1);

        glfwSwapBuffers(window);
        // platform_sleep(10);
        glfwPollEvents();
    }

    tay_simulation_end(tay);
    tay_threads_stop(); // TODO: remove this!!!
    tay_destroy_state(tay);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
