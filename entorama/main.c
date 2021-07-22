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
static int paused = 1;
static int window_w = 1600;
static int window_h = 800;

static int mouse_started_moving = 0;
static float mouse_dx = 0.0f;
static float mouse_dy = 0.0f;

static int TOOLBAR_H = 40;
static int STATUSBAR_H = 26;
static float SIDEBAR_W = 300.0f;

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

    if (!em_widget_pressed())
        drawing_mouse_move(mouse_l, mouse_r, mouse_dx, mouse_dy);
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
    em_widgets_init();

    TayState *tay = tay_create_state();

    entorama_init_model(&model);
    model_load(&model, "m_sph.dll");
    model.init(&model, tay);
    model.reset(&model, tay);

    tay_threads_start(100000); // TODO: remove this!!!
    tay_simulation_start(tay);

    drawing_init(1000000);

    drawing_update_world_box(&model);

    while (!quit) {

        if (!paused)
            tay_run(tay, 1);

        /* drawing */
        {
            graphics_viewport((int)SIDEBAR_W, STATUSBAR_H, window_w - (int)SIDEBAR_W, window_h - TOOLBAR_H - STATUSBAR_H);
            vec4 bg = color_bg();
            graphics_clear(bg.x, bg.y, bg.z);
            graphics_clear_depth();

            /* draw agents */
            {
                graphics_enable_depth_test(1);
                drawing_camera_setup(&model, window_w - (int)SIDEBAR_W, window_h - TOOLBAR_H - STATUSBAR_H);

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

                em_quad(0.0f, (float)(window_h - TOOLBAR_H), (float)window_w, (float)window_h, color_vd());
                em_quad(0.0f, 0.0f, (float)window_w, (float)STATUSBAR_H, color_vd());

                /* toolbar */
                {
                    switch (em_button("Run",
                                      (window_w - 60) * 0.5f, (float)(window_h - TOOLBAR_H),
                                      (window_w + 60) * 0.5f, (float)window_h,
                                      EM_BUTTON_FLAGS_NONE)) {
                        case EM_RESPONSE_CLICKED:
                            paused = !paused;
                        case EM_RESPONSE_HOVERED:
                            sprintf_s(tooltip_text_buffer, MAX_TOOLTIP_TEXT_BUFFER, "Run/pause simulation");
                        default:;
                    }

                    if (em_button("Theme",
                                  window_w - 60.0f, (float)(window_h - TOOLBAR_H),
                                  (float)window_w, (float)window_h,
                                  EM_BUTTON_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                        color_toggle_theme();
                }

                /* sidebar */
                {
                    em_quad(0.0f, (float)STATUSBAR_H, SIDEBAR_W, (float)(window_h - TOOLBAR_H), color_vd());

                    if (em_button("",
                                  SIDEBAR_W, (float)STATUSBAR_H,
                                  SIDEBAR_W + 6.0f, (float)(window_h - TOOLBAR_H),
                                  EM_BUTTON_FLAGS_NONE) == EM_RESPONSE_PRESSED)
                        SIDEBAR_W += mouse_dx;

                    const float SIDEBAR_BUTTON_H = 52.0f;
                    float y = window_h - TOOLBAR_H - SIDEBAR_BUTTON_H;

                    /* group buttons */
                    {
                        for (unsigned group_i = 0; group_i < model.groups_count; ++group_i) {
                            EntoramaGroup *group = model.groups + group_i;

                            if (em_button(group->name,
                                          0.0f, y,
                                          SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                          (selected_model_element == group) ? EM_BUTTON_FLAGS_PRESSED : EM_BUTTON_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                                selected_model_element = group;

                            y -= SIDEBAR_BUTTON_H;
                        }
                    }

                    /* pass buttons */
                    {
                        const float bullet_size = 8.0f;
                        const float bullet_offset = 16.0f;

                        float bottom_bullet_y = 0.0f;
                        float top_bullet_y = 0.0f;

                        em_set_button_label_offset(bullet_offset * 2.0f + bullet_size);

                        for (unsigned pass_i = 0; pass_i < model.passes_count; ++pass_i) {
                            EntoramaPass *pass = model.passes + pass_i;

                            if (em_button(pass->name,
                                          0.0f, y,
                                          SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                          (selected_model_element == pass) ? EM_BUTTON_FLAGS_PRESSED : EM_BUTTON_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                                selected_model_element = pass;

                            float bullet_y = y + SIDEBAR_BUTTON_H * 0.5f;
                            if (pass_i == 0)
                                top_bullet_y = bullet_y;
                            bottom_bullet_y = bullet_y;

                            em_quad(bullet_offset, y + (SIDEBAR_BUTTON_H - bullet_size) * 0.5f,
                                    bullet_offset + bullet_size, y + (SIDEBAR_BUTTON_H + bullet_size) * 0.5f,
                                    color_palette(3));

                            y -= SIDEBAR_BUTTON_H;
                        }

                        em_quad(bullet_offset + bullet_size * 0.5f - 0.5f, bottom_bullet_y,
                                bullet_offset + bullet_size * 0.5f + 0.5f, top_bullet_y,
                                color_palette(3));

                        em_reset_button_label_offset();
                    }

                    if (em_area("Sidebar background",
                                0.0f, (float)STATUSBAR_H,
                                SIDEBAR_W, (float)(window_h - TOOLBAR_H)) == EM_RESPONSE_CLICKED)
                        selected_model_element = 0;
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
                                      EM_BUTTON_FLAGS_NONE)) {
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

                em_quad(0.0f, (float)STATUSBAR_H, (float)window_w, STATUSBAR_H + 1.0f, color_border());
                em_quad(0.0f, window_h - TOOLBAR_H - 1.0f, (float)window_w, (float)(window_h - TOOLBAR_H), color_border());
                em_quad((float)SIDEBAR_W, (float)STATUSBAR_H, SIDEBAR_W + 1.0f, (float)(window_h - TOOLBAR_H), color_border());

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
