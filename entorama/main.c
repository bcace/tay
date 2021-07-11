#include "main.h"
#include "entorama.h"
#include "tay.h"
#include "thread.h" // TODO: remove this!!!
#include "graphics.h"
#include "font.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>


static int quit = 0;
static int paused = 1;
static int window_w = 1600;
static int window_h = 800;

static int mouse_started_moving = 0;
static int mouse_l = 0;
static int mouse_r = 0;
static float mouse_x;
static float mouse_y;
static float mouse_dx = 0.0f;
static float mouse_dy = 0.0f;


static void _close_callback(GLFWwindow *window) {
    quit = 1;
}

static void _scroll_callback(GLFWwindow *glfw_window, double x, double y) {
    drawing_mouse_scroll(y);
}

static void _mousebutton_callback(GLFWwindow *glfw_window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouse_l = 1;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            mouse_r = 1;
    }
    else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            mouse_l = 0;
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            mouse_r = 0;
    }
}

static void _mousepos_callback(GLFWwindow *glfw_window, double x, double y) {
    y = window_h - y;
    if (mouse_started_moving) {
        mouse_dx = (float)x - mouse_x;
        mouse_dy = (float)y - mouse_y;
    }
    else {
        mouse_dx = 0.0f;
        mouse_dy = 0.0f;
        mouse_started_moving = 1;
    }
    mouse_x = (float)x;
    mouse_y = (float)y;

    drawing_mouse_move(mouse_l, mouse_r, mouse_dx, mouse_dy);
}

static void _key_callback(GLFWwindow *glfw_window, int key, int code, int action, int mods) {
    if (key == GLFW_KEY_Q && mods & GLFW_MOD_CONTROL)
        quit = 1;
    else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        paused = !paused;
}

static double _smooth_ms_per_step(double ms) {
    static double smooth = 0.0;
    double ratio = ms / smooth;
    if (ratio > 1.5 || ratio < 0.5)
        smooth = ms;
    else
        smooth = smooth * 0.99 + ms * 0.01;
    return smooth;
}

int main() {

    if (!glfwInit()) {
        fprintf(stderr, "Could not initialize GLFW\n");
        return 1;
    }

    GLFWmonitor *monitor = 0;

#if 0
    monitor = glfwGetPrimaryMonitor();
    GLFWvidmode *mode = glfwGetVideoMode(monitor);
    window_w = mode->width;
    window_h = mode->height;
#endif

    GLFWwindow *window = glfwCreateWindow(window_w, window_h, "Entorama", monitor, 0);

    if (!window) {
        fprintf(stderr, "Could not create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowCloseCallback(window, _close_callback);
    glfwSetScrollCallback(window, _scroll_callback);
    glfwSetCursorPosCallback(window, _mousepos_callback);
    glfwSetMouseButtonCallback(window, _mousebutton_callback);
    glfwSetKeyCallback(window, _key_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    graphics_enable_blend(1);

    font_init();

    TayState *tay = tay_create_state();

    EntoramaModel model;
    entorama_init_model(&model);
    model_load(&model, "m_flocking.dll");
    model.init(&model, tay);

    tay_threads_start(100000); // TODO: remove this!!!
    tay_simulation_start(tay);

    drawing_init(1000000);

    while (!quit) {

        if (!paused)
            tay_run(tay, 1);

        /* drawing */
        {
            graphics_viewport(0, 0, window_w, window_h);
            vec4 bg = color_bg();
            graphics_clear(bg.x, bg.y, bg.z);
            graphics_clear_depth();
            graphics_enable_depth_test(1);

            drawing_camera_setup(&model, window_w, window_h);

            /* draw agents */
            for (unsigned group_i = 0; group_i < model.groups_count; ++group_i) {
                EntoramaGroup *group = model.groups + group_i;
                drawing_draw_group(tay, group, group_i);
            }

            /* draw flat overlay */
            {
                mat4 projection;
                graphics_ortho(&projection, 0.0f, (float)window_w, 0.0f, (float)window_h, -100.0f, 100.0f);

                font_use_medium();

                char buffer[50];
                sprintf_s(buffer, 50, "ms: %.1f", _smooth_ms_per_step(tay_get_ms_per_step_for_last_run(tay)));

                font_draw_text(buffer, window_w - font_text_length(buffer) - 10, 10, projection, color_fg());
            }

            glfwSwapBuffers(window);
        }

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
