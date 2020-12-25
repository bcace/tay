#include "window.h"
#include "graphics.h"
#include "platform.h"
#include "shaders.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <stdlib.h>


Program program;

static struct TayState *state = 0;
static float *vertices = 0;
static float *colors = 0;

typedef struct Window {
    int w, h;
    int quit;
} Window;

Window window;

static void _close_callback(GLFWwindow *glfw_window) {
    window.quit = 1;
}

static void _main_loop_func(GLFWwindow *glfw_window) {
    graphics_viewport(0, 0, window.w, window.h);
    graphics_clear(0.0f, 0.0f, 0.0f);
    graphics_clear_depth();
    graphics_enable_depth_test(1);

    shader_program_use(&program);

    mat4 projection;
    // graphics_ortho(&projection, -window.w * 0.5f, window.w * 0.5f, -window.h * 0.5f, window.h * 0.5f, -100.0f, 1000.0f);
    graphics_perspective(&projection, 1.5f, window.w / (float)window.h, 0.1f, 1000.0f);
    shader_program_set_uniform_mat4(&program, 0, &projection);

    mat4 modelview;
    mat4_set_identity(&modelview);
    mat4_translate(&modelview, 0.0f, 0.0f, -100.0f);
    shader_program_set_uniform_mat4(&program, 1, &modelview);

    // draw agents here
    
    glfwSwapBuffers(glfw_window);

    // go to next frame

    platform_sleep(10);
    glfwPollEvents();
}

void window_open() {
    window.w = 1800;
    window.h = 1000;
    window.quit = 0;

    const int max_agents = 1000000;
    vertices = calloc(max_agents * 3, sizeof(float));
    colors = calloc(max_agents * 4, sizeof(float));

    if (!glfwInit()) {
        printf("Could not initialize GLFW\n");
        return;
    }
    GLFWmonitor *monitor = 0;
    GLFWwindow *glfw_window = glfwCreateWindow(window.w, window.h, "Tay", monitor, 0);
    if (!glfw_window) {
        printf("Could not create GLFW window\n");
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(glfw_window);

    glfwSetWindowCloseCallback(glfw_window, _close_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    // glEnable(GL_LINE_SMOOTH);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_program_init(&program, flat_vert, "flat.vert", flat_frag, "flat.frag");
    shader_program_define_in_float(&program, 3); /* position */
    shader_program_define_in_float(&program, 4); /* color */
    shader_program_define_uniform(&program, "projection");
    shader_program_define_uniform(&program, "modelview");

    graphics_point_size(5.0f);

    while (!window.quit)
        _main_loop_func(glfw_window);

    glfwDestroyWindow(glfw_window);
    glfwTerminate();
}
