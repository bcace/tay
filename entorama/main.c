#include "main.h"
#include "entorama.h"
#include "tay.h"
#include "thread.h" // TODO: remove this!!!
#include "widgets.h"
#include "graphics.h"
#include "font.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>


static int quit = 0;
static int window_w = 1600;
static int window_h = 800;

int mouse_l = 0;
int mouse_r = 0;
float mouse_x;
float mouse_y;

static int mouse_started_moving = 0;
static float mouse_dx = 0.0f;
static float mouse_dy = 0.0f;

const int TOOLBAR_H = 40;
const int STATUSBAR_H = 26;
const int SIDEBAR_W = 320;

static EntoramaModel model;
static void *selected_model_element = 0;

#define MAX_SPEED_TEXT_BUFFER 16
static char speed_text_buffer[MAX_SPEED_TEXT_BUFFER];
static int speed_mode = 0;

#define MAX_TOOLTIP_TEXT_BUFFER 512
static char tooltip_text_buffer[MAX_TOOLTIP_TEXT_BUFFER];

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

    widgets_mouse_button(button == GLFW_MOUSE_BUTTON_LEFT ? 0 : 1, action == GLFW_PRESS ? 0 : 1, mouse_x, mouse_y);
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
    widgets_mouse_move(mouse_l, mouse_r, mouse_x, mouse_y);
}

static void _key_callback(GLFWwindow *glfw_window, int key, int code, int action, int mods) {
    if (key == GLFW_KEY_Q && mods & GLFW_MOD_CONTROL)
        quit = 1;
    else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        paused = !paused;
}

static void _size_callback(GLFWwindow *glfw_window, int w, int h) {
    window_w = w;
    window_h = h;
    widgets_update(window_w, window_h, TOOLBAR_H, STATUSBAR_H, SIDEBAR_W);
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
    // glfwMaximizeWindow(window);
    // glfwGetWindowSize(window, &window_w, &window_h);

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
    glfwSetWindowSizeCallback(window, _size_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress); /* load extensions */

    graphics_enable_blend(1);

    color_init();
    font_init();
    widgets_init();
    // widgets_update(window_w, window_h, TOOLBAR_H, STATUSBAR_H, SIDEBAR_W);

    TayState *tay = tay_create_state();

    entorama_init_model(&model);
    model_load(&model, "m_sph.dll");
    model.init(&model, tay);
    model.reset(&model, tay);

    widgets_update_model_specific(&model);

    tay_threads_start(100000); // TODO: remove this!!!
    tay_simulation_start(tay);

    drawing_init(1000000);

    drawing_update_world_box(&model);

    while (!quit) {

        if (!paused)
            tay_run(tay, 1);

        /* drawing */
        {
            graphics_viewport(SIDEBAR_W, STATUSBAR_H, window_w - SIDEBAR_W, window_h - TOOLBAR_H - STATUSBAR_H);
            vec4 bg = color_bg();
            graphics_clear(bg.x, bg.y, bg.z);
            graphics_clear_depth();

            /* draw agents */
            {
                graphics_enable_depth_test(1);
                drawing_camera_setup(&model, window_w - SIDEBAR_W, window_h - TOOLBAR_H - STATUSBAR_H);

                for (unsigned group_i = 0; group_i < model.groups_count; ++group_i) {
                    EntoramaGroup *group = model.groups + group_i;
                    drawing_draw_group(tay, group, group_i);
                }
            }

            /* draw widgets */
            {
                graphics_viewport(0, 0, window_w, window_h);

                mat4 projection;
                graphics_ortho(&projection, 0.0f, (float)window_w, 0.0f, (float)window_h, -100.0f, 100.0f);

                tooltip_text_buffer[0] = '\0';

                em_widgets_begin();

                switch (em_button("Run",
                                  (window_w - 60) * 0.5f, (float)(window_h - TOOLBAR_H),
                                  (window_w + 60) * 0.5f, (float)window_h,
                                  EM_BUTTON_STATE_NONE)) {
                    case EM_RESPONSE_CLICKED:
                        paused = !paused;
                    case EM_RESPONSE_HOVERED:
                        sprintf_s(tooltip_text_buffer, MAX_TOOLTIP_TEXT_BUFFER, "Run/pause simulation");
                    default:;
                }

                /* sidebar */
                {
                    if (em_area("Sidebar background",
                                0.0f, (float)STATUSBAR_H,
                                (float)SIDEBAR_W, (float)(window_h - TOOLBAR_H)) == EM_RESPONSE_CLICKED)
                        selected_model_element = 0;

                    const float SIDEBAR_BUTTON_H = 52.0f;
                    float y = window_h - TOOLBAR_H - SIDEBAR_BUTTON_H;

                    for (unsigned group_i = 0; group_i < model.groups_count; ++group_i) {
                        EntoramaGroup *group = model.groups + group_i;

                        if (em_button(group->name,
                                           0.0f, y,
                                           (float)SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                           (selected_model_element == group) ? EM_BUTTON_STATE_PRESSED : EM_BUTTON_STATE_NONE) == EM_RESPONSE_CLICKED)
                            selected_model_element = group;

                        y -= SIDEBAR_BUTTON_H;
                    }

                    for (unsigned pass_i = 0; pass_i < model.passes_count; ++pass_i) {
                        EntoramaPass *pass = model.passes + pass_i;

                        if (em_button(pass->name,
                                           0.0f, y,
                                           (float)SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                           (selected_model_element == pass) ? EM_BUTTON_STATE_PRESSED : EM_BUTTON_STATE_NONE) == EM_RESPONSE_CLICKED)
                            selected_model_element = pass;

                        y -= SIDEBAR_BUTTON_H;
                    }
                }

                /* statusbar */
                {
                    double speed = _smooth_ms_per_step(tay_get_ms_per_step_for_last_run(tay));
                    if (speed_mode)
                        sprintf_s(speed_text_buffer, MAX_SPEED_TEXT_BUFFER, "%.1f fps", 1000.0 / speed);
                    else
                        sprintf_s(speed_text_buffer, MAX_SPEED_TEXT_BUFFER, "%.1f ms", speed);

                    switch (em_button(speed_text_buffer,
                                      window_w - font_text_width(ENTORAMA_FONT_MEDIUM, speed_text_buffer) - 20.0f, 0.0f,
                                      (float)window_w, (float)STATUSBAR_H,
                                      EM_BUTTON_STATE_NONE)) {
                        case EM_RESPONSE_CLICKED:
                            speed_mode = !speed_mode;
                        case EM_RESPONSE_HOVERED:
                            sprintf_s(tooltip_text_buffer, MAX_TOOLTIP_TEXT_BUFFER, "Toggle milliseconds per step / frames per second");
                        default:;
                    }

                    em_label(tooltip_text_buffer,
                             0.0f, 0.0f,
                             (float)window_w, (float)STATUSBAR_H);
                }

                em_widgets_end(projection);
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
