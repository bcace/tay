#include "main.h"
#include "entorama.h"
#include "tay.h"
#include "widgets.h"
#include "graphics.h"
#include "platform.h"
#include "font.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <windows.h>


static int quit = 0;
static int running = 0;
static int window_w = 1600;
static int window_h = 800;

static int mouse_started_moving = 0;
static float mouse_dx = 0.0f;
static float mouse_dy = 0.0f;

static float TOOLBAR_H = 30;
static float STATUSBAR_H = 26;
static float SIDEBAR_W = 300.0f;

static EmModel model;
static EmIface iface;

static int speed_mode = 0;
static unsigned thread_storage_size = 100000;
static int simulation_expanded = 0;
static int devices_expanded = 0;
static int cpu_expanded = 0;
static int gpu_expanded = 0;

static char label_text_buffer[512];
static char tooltip_text_buffer[512];

static HANDLE run_thread;
static HANDLE run_thread_semaphore;
static HANDLE main_thread_semaphore;

static double run_thread_ms = 0.0;
static double main_thread_ms = 0.0;

static TayState *tay;

typedef enum {
    COMM_NONE,
    COMM_RUN,
    COMM_STOP,
    COMM_RELOAD,
} Command;

static Command command;

static void _close_callback(GLFWwindow *window) {
    quit = 1;
}

static void _scroll_callback(GLFWwindow *glfw_window, double x, double y) {
    drawing_mouse_scroll(y, &redraw);
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
        drawing_mouse_move(mouse_l, mouse_r, mouse_dx, mouse_dy, &redraw);
}

static void _key_callback(GLFWwindow *glfw_window, int key, int code, int action, int mods) {
    if (key == GLFW_KEY_Q && mods & GLFW_MOD_CONTROL)
        quit = 1;
    else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
        if (running)
            command = COMM_STOP;
        else
            command = COMM_RUN;
    }
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

static void _reconfigure_space(TayState *tay, EmGroup *group) {
    tay_configure_space(tay, group->group, group->space_type, 3, (float4){group->min_part_size_x, group->min_part_size_y, group->min_part_size_z, group->min_part_size_w}, 1000);
}

static const char *_structure_name(TaySpaceType space_type) {
    switch (space_type) {
        case TAY_CPU_SIMPLE: return "Simple";
        case TAY_CPU_KD_TREE: return "Kd Tree";
        case TAY_CPU_AABB_TREE: return "AABB Tree";
        case TAY_CPU_GRID: return "Grid";
        case TAY_CPU_Z_GRID: return "Z-Order Grid";
        default: return "(unknown)";
    }
}

static int _structure_works_on_cpu(TaySpaceType space_type) {
    switch (space_type) {
        case TAY_CPU_SIMPLE: return 1;
        case TAY_CPU_KD_TREE: return 1;
        case TAY_CPU_AABB_TREE: return 1;
        case TAY_CPU_GRID: return 1;
        case TAY_CPU_Z_GRID: return 1;
        default: return 0;
    }
}

static int _structure_works_on_gpu(TaySpaceType space_type) {
    switch (space_type) {
        case TAY_CPU_SIMPLE: return 1;
        case TAY_CPU_KD_TREE: return 0;
        case TAY_CPU_AABB_TREE: return 0;
        case TAY_CPU_GRID: return 0;
        case TAY_CPU_Z_GRID: return 1;
        default: return 0;
    }
}

static int _structure_works_with_points(TaySpaceType space_type) {
    switch (space_type) {
        case TAY_CPU_SIMPLE: return 1;
        case TAY_CPU_KD_TREE: return 1;
        case TAY_CPU_AABB_TREE: return 0;
        case TAY_CPU_GRID: return 1;
        case TAY_CPU_Z_GRID: return 1;
        default: return 0;
    }
}

static int _structure_works_with_non_points(TaySpaceType space_type) {
    switch (space_type) {
        case TAY_CPU_SIMPLE: return 1;
        case TAY_CPU_KD_TREE: return 1;
        case TAY_CPU_AABB_TREE: return 1;
        case TAY_CPU_GRID: return 0;
        case TAY_CPU_Z_GRID: return 0;
        default: return 0;
    }
}

static unsigned int WINAPI _thread_func(void *data) {
    while (1) {
        WaitForSingleObject(run_thread_semaphore, INFINITE);
        if (quit) {
            ReleaseSemaphore(main_thread_semaphore, 1, 0);
            return 0;
        }
        double sum_ms = 0.0;
        while (sum_ms < 9.0) {
            tay_run(tay, 1);
            run_thread_ms = tay_get_ms_per_step_for_last_run(tay);
            sum_ms += run_thread_ms;
        }
        ReleaseSemaphore(main_thread_semaphore, 1, 0);
    }
    return 0;
}

static void _copy_drawing_data_from_agents(EmModel *model, TayState *tay) {
    for (unsigned group_i = 0; group_i < model->groups_count; ++group_i) {
        EmGroup *group = model->groups + group_i;
        drawing_fill_group_buffers(group, tay);
    }
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

#if 0
    glfwMaximizeWindow(window);
    glfwGetWindowSize(window, &window_w, &window_h);
#endif

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

    tay = tay_create_state();

    entorama_load_model_dll(&iface, &model, "m_sph.dll");
    iface.init(&model, tay);
    iface.reset(&model, tay);

    redraw = 1;

    for (unsigned group_i = 0; group_i < model.groups_count; ++group_i)
        _reconfigure_space(tay, model.groups + group_i);

    tay_threads_start(0, thread_storage_size);
    tay_simulation_start(tay);

    drawing_init(1000000);

    for (unsigned group_i = 0; group_i < model.groups_count; ++group_i) {
        EmGroup *group = model.groups + group_i;
        drawing_create_group_buffers(group);
        drawing_fill_group_buffers(group, tay);
    }

    drawing_update_world_box(&model);

    main_thread_semaphore = CreateSemaphore(0, 0, 1, 0);
    run_thread_semaphore = CreateSemaphore(0, 0, 1, 0);
    run_thread = CreateThread(0, 0, _thread_func, 0, 0, 0);

    command = COMM_NONE;

    while (!quit) {

        DWORD unblock_reason = WaitForSingleObject(main_thread_semaphore, 10);

        if (unblock_reason == WAIT_OBJECT_0) { /* unblocked because run thread was done */

            main_thread_ms = run_thread_ms;

            /* process commands while running */
            if (command == COMM_STOP) {
                command = COMM_NONE;
                running = 0;
            }
            else if (command == COMM_RELOAD) {
                iface.reset(&model, tay);
                ReleaseSemaphore(run_thread_semaphore, 1, 0);
                command = COMM_NONE;
            }
            else
                ReleaseSemaphore(run_thread_semaphore, 1, 0);

            _copy_drawing_data_from_agents(&model, tay);
            redraw = 1;
        }
        else { /* unblocked because of timeout */

            /* process commands while paused */
            if (command == COMM_RUN) {
                ReleaseSemaphore(run_thread_semaphore, 1, 0);
                command = COMM_NONE;
                running = 1;
            }
            else if (command == COMM_RELOAD) {
                if (!running) {
                    iface.reset(&model, tay);
                    _copy_drawing_data_from_agents(&model, tay);
                    redraw = 1;
                    command = COMM_NONE;
                }
            }
        }

        /* widgets */
        {
            tooltip_text_buffer[0] = '\0';

            em_widgets_begin();

            em_select_layer(2);

            em_quad(0.0f, window_h - TOOLBAR_H, (float)window_w, (float)window_h, color_vd());
            em_quad(0.0f, 0.0f, (float)window_w, STATUSBAR_H, color_vd());

            /* toolbar */
            {
                const float TOOLBAR_BUTTON_W = TOOLBAR_H;

                float button_x = (window_w - TOOLBAR_BUTTON_W * 2.0f) * 0.5f;

                switch (em_button_with_icon("", running ? 7 : 6,
                                            button_x, window_h - TOOLBAR_H,
                                            button_x + TOOLBAR_BUTTON_W, (float)window_h,
                                            EM_WIDGET_FLAGS_ICON_ONLY | ((command == COMM_NONE) ? EM_WIDGET_FLAGS_NONE : EM_WIDGET_FLAGS_DISABLED))) {
                    case EM_RESPONSE_CLICKED: {
                        if (running)
                            command = COMM_STOP;
                        else
                            command = COMM_RUN;
                    }
                    case EM_RESPONSE_HOVERED:
                        sprintf_s(tooltip_text_buffer, sizeof(tooltip_text_buffer), "Run / pause simulation");
                    default:;
                }

                button_x += TOOLBAR_BUTTON_W;

                switch (em_button_with_icon("", 8,
                                            button_x, window_h - TOOLBAR_H,
                                            button_x + TOOLBAR_BUTTON_W, (float)window_h,
                                            EM_WIDGET_FLAGS_ICON_ONLY | ((command == COMM_NONE) ? EM_WIDGET_FLAGS_NONE : EM_WIDGET_FLAGS_DISABLED))) {
                    case EM_RESPONSE_CLICKED:
                        command = COMM_RELOAD;
                    case EM_RESPONSE_HOVERED:
                        sprintf_s(tooltip_text_buffer, sizeof(tooltip_text_buffer), "Reset simulation");
                    default:;
                }

                if (em_button_with_icon("", 9,
                                        window_w - TOOLBAR_BUTTON_W, window_h - TOOLBAR_H,
                                        (float)window_w, (float)window_h,
                                        EM_WIDGET_FLAGS_ICON_ONLY) == EM_RESPONSE_CLICKED)
                    color_toggle_theme();
            }

            /* sidebar */
            {
                /* sidebar size button */
                if (em_button("",
                              SIDEBAR_W, STATUSBAR_H,
                              SIDEBAR_W + 6.0f, window_h - TOOLBAR_H,
                              EM_WIDGET_FLAGS_NONE) == EM_RESPONSE_PRESSED)
                    SIDEBAR_W += mouse_dx;

                /* root level */
                {
                    em_select_layer(0);
                    em_set_layer_scissor(0.0f, STATUSBAR_H, SIDEBAR_W, window_h - TOOLBAR_H);

                    em_select_layer(1);
                    em_set_layer_scissor(0.0f, STATUSBAR_H, SIDEBAR_W, window_h - TOOLBAR_H);

                    em_select_layer(0);

                    /* background */
                    em_quad(0.0f, STATUSBAR_H, SIDEBAR_W, window_h - TOOLBAR_H, color_vd());

                    const float INDENT_W = 16.0f;
                    const float SIDEBAR_BUTTON_H = font_height(EM_FONT_MEDIUM) + 6.0f;
                    float y = window_h - TOOLBAR_H - SIDEBAR_BUTTON_H - 8.0f;
                    float x = 0.0f;

                    /* simulation tree */
                    {
                        if (em_button_with_icon("Simulation", simulation_expanded,
                                                x, y,
                                                SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                                EM_WIDGET_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                            simulation_expanded = !simulation_expanded;

                        y -= SIDEBAR_BUTTON_H;

                        if (simulation_expanded) {

                            x += INDENT_W;

                            /* devices */
                            {
                                if (em_button_with_icon("Devices", devices_expanded,
                                                        INDENT_W, y,
                                                        SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                                        EM_WIDGET_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                                    devices_expanded = !devices_expanded;

                                y -= SIDEBAR_BUTTON_H;

                                if (devices_expanded) {

                                    x += INDENT_W;

                                    /* cpu device */
                                    {
                                        EmWidgetFlags flags = EM_WIDGET_FLAGS_NONE;
                                        if (!model.ocl_enabled)
                                            flags |= EM_WIDGET_FLAGS_PRESSED;

                                        if (em_button_with_icon("CPU", cpu_expanded,
                                                                x, y,
                                                                SIDEBAR_W - SIDEBAR_BUTTON_H, y + SIDEBAR_BUTTON_H,
                                                                EM_WIDGET_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                                            cpu_expanded = !cpu_expanded;

                                        em_select_layer(1);

                                        if (em_button_with_icon("", model.ocl_enabled ? 3 : 2,
                                                                SIDEBAR_W - SIDEBAR_BUTTON_H, y,
                                                                SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                                                EM_WIDGET_FLAGS_ICON_ONLY) == EM_RESPONSE_CLICKED)
                                            model.ocl_enabled = tay_switch_to_host(tay);

                                        em_select_layer(0);

                                        y -= SIDEBAR_BUTTON_H;

                                        if (cpu_expanded) {

                                            x += INDENT_W;

                                            /* threads count */
                                            {
                                                em_label("CPU threads",
                                                         x, y,
                                                         SIDEBAR_W * 0.5f, y + SIDEBAR_BUTTON_H,
                                                         EM_WIDGET_FLAGS_NONE);

                                                {
                                                    unsigned threads_count = tay_get_number_of_threads();
                                                    sprintf_s(label_text_buffer, sizeof(label_text_buffer), "%d", threads_count);

                                                    float x = SIDEBAR_W * 0.5f;

                                                    if (em_button_with_icon("", 5,
                                                                            x, y,
                                                                            SIDEBAR_W * 0.5f + SIDEBAR_BUTTON_H, y + SIDEBAR_BUTTON_H,
                                                                            EM_WIDGET_FLAGS_ICON_ONLY) == EM_RESPONSE_CLICKED) {
                                                        tay_threads_stop();
                                                        tay_threads_start(--threads_count, thread_storage_size);
                                                    }

                                                    float label_w = font_width(EM_FONT_MEDIUM) * 5.0f;

                                                    x += SIDEBAR_BUTTON_H;

                                                    em_label(label_text_buffer,
                                                             x, y,
                                                             x + label_w, y + SIDEBAR_BUTTON_H,
                                                             EM_WIDGET_FLAGS_ICON_ONLY);

                                                    x += label_w;

                                                    if (em_button_with_icon("", 4,
                                                                            x, y,
                                                                            x + SIDEBAR_BUTTON_H, y + SIDEBAR_BUTTON_H,
                                                                            EM_WIDGET_FLAGS_ICON_ONLY) == EM_RESPONSE_CLICKED) {
                                                        tay_threads_stop();
                                                        tay_threads_start(++threads_count, thread_storage_size);
                                                    }
                                                }

                                                y -= SIDEBAR_BUTTON_H;
                                            }

                                            x -= INDENT_W;
                                        }
                                    }

                                    /* gpu device */
                                    {
                                        EmWidgetFlags flags = EM_WIDGET_FLAGS_ICON_ONLY;
                                        if (!model.ocl_enabled) {
                                            for (unsigned group_i = 0; group_i < model.groups_count; ++group_i) {
                                                EmGroup *group = model.groups + group_i;

                                                if (group->space_type != TAY_CPU_SIMPLE && group->space_type != TAY_CPU_Z_GRID) {
                                                    flags |= EM_WIDGET_FLAGS_DISABLED;
                                                    // TODO: set tooltip explanation
                                                    break;
                                                }
                                            }
                                        }

                                        if (em_button_with_icon("GPU", gpu_expanded,
                                                                x, y,
                                                                SIDEBAR_W - SIDEBAR_BUTTON_H, y + SIDEBAR_BUTTON_H,
                                                                EM_WIDGET_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                                            gpu_expanded = !gpu_expanded;

                                        em_select_layer(1);

                                        if (em_button_with_icon("", model.ocl_enabled ? 2 : 3,
                                                                SIDEBAR_W - SIDEBAR_BUTTON_H, y,
                                                                SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                                                flags) == EM_RESPONSE_CLICKED)
                                            model.ocl_enabled = tay_switch_to_ocl(tay);

                                        em_select_layer(0);

                                        y -= SIDEBAR_BUTTON_H;
                                    }

                                    x -= INDENT_W;
                                }
                            }

                            x -= INDENT_W;
                        }
                    }

                    /* group buttons */
                    {
                        for (unsigned group_i = 0; group_i < model.groups_count; ++group_i) {
                            EmGroup *group = model.groups + group_i;

                            if (em_button_with_icon(group->name, group->expanded,
                                                    0.0f, y,
                                                    SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                                    EM_WIDGET_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                                group->expanded = !group->expanded;

                            em_quad(0.0f, y,
                                    4.0f, y + SIDEBAR_BUTTON_H,
                                    color_palette(group_i));

                            y -= SIDEBAR_BUTTON_H;

                            if (group->expanded) {

                                x += INDENT_W;

                                /* structures header */
                                {
                                    sprintf_s(label_text_buffer, sizeof(label_text_buffer), "Structure types (%s)", _structure_name(group->space_type));

                                    if (em_button_with_icon(label_text_buffer, group->structures_expanded,
                                                            x, y,
                                                            SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                                            EM_WIDGET_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                                        group->structures_expanded = !group->structures_expanded;

                                    y -= SIDEBAR_BUTTON_H;
                                }

                                /* individual structures */
                                if (group->structures_expanded) {

                                    x += INDENT_W;

                                    for (TaySpaceType space_type = TAY_CPU_SIMPLE; space_type < TAY_SPACE_COUNT; ++space_type) {

                                        sprintf_s(label_text_buffer, sizeof(label_text_buffer), _structure_name(space_type));

                                        EmWidgetFlags flags = EM_WIDGET_FLAGS_NONE;

                                        if (group->is_point && !_structure_works_with_points(space_type) ||
                                            !group->is_point && !_structure_works_with_non_points(space_type) ||
                                            model.ocl_enabled && !_structure_works_on_gpu(space_type))
                                            flags |= EM_WIDGET_FLAGS_DISABLED;

                                        em_label(label_text_buffer,
                                                 x, y,
                                                 SIDEBAR_W - SIDEBAR_BUTTON_H, y + SIDEBAR_BUTTON_H,
                                                 flags);

                                        if (em_button_with_icon("", (space_type == group->space_type) ? 2 : 3,
                                                                SIDEBAR_W - SIDEBAR_BUTTON_H, y,
                                                                SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                                                flags | EM_WIDGET_FLAGS_ICON_ONLY) == EM_RESPONSE_CLICKED) {
                                            group->space_type = space_type;
                                            _reconfigure_space(tay, group);
                                        }

                                        y -= SIDEBAR_BUTTON_H;
                                    }

                                    x -= INDENT_W;
                                }

                                x -= INDENT_W;
                            }
                        }
                    }

                    /* pass buttons */
                    {
                        for (unsigned pass_i = 0; pass_i < model.passes_count; ++pass_i) {
                            EmPass *pass = model.passes + pass_i;

                            if (pass->type == EM_PASS_ACT)
                                sprintf_s(label_text_buffer, sizeof(label_text_buffer), "%s act (%s)", pass->act_group->name, pass->name);
                            else
                                sprintf_s(label_text_buffer, sizeof(label_text_buffer), "%s see %s (%s)", pass->seer_group->name, pass->seen_group->name, pass->name);

                            if (em_button(label_text_buffer,
                                          0.0f, y,
                                          SIDEBAR_W, y + SIDEBAR_BUTTON_H,
                                          EM_WIDGET_FLAGS_NONE) == EM_RESPONSE_CLICKED)
                                pass->expanded = !pass->expanded;

                            y -= SIDEBAR_BUTTON_H;
                        }
                    }
                }

                em_select_layer(2);
            }

            /* statusbar */
            {
                double speed = _smooth_ms_per_step(main_thread_ms);
                if (speed_mode)
                    sprintf_s(label_text_buffer, sizeof(label_text_buffer), "%.1f fps", 1000.0 / speed);
                else
                    sprintf_s(label_text_buffer, sizeof(label_text_buffer), "%.1f ms", speed);

                switch (em_button(label_text_buffer,
                                  window_w - font_text_width(EM_FONT_MEDIUM, label_text_buffer) - 20.0f, 0.0f,
                                  (float)window_w, STATUSBAR_H,
                                  EM_WIDGET_FLAGS_NONE)) {
                    case EM_RESPONSE_CLICKED:
                        speed_mode = !speed_mode;
                    case EM_RESPONSE_HOVERED:
                        sprintf_s(tooltip_text_buffer, sizeof(tooltip_text_buffer), "Toggle milliseconds per step / frames per second");
                    default:;
                }

                em_label(tooltip_text_buffer,
                         0.0f, 0.0f,
                         (float)window_w, STATUSBAR_H,
                         EM_WIDGET_FLAGS_NONE);
            }

            em_quad(0.0f, STATUSBAR_H, (float)window_w, STATUSBAR_H + 1.0f, color_border());
            em_quad(0.0f, window_h - TOOLBAR_H - 1.0f, (float)window_w, window_h - TOOLBAR_H, color_border());
            em_quad(SIDEBAR_W, STATUSBAR_H, SIDEBAR_W + 1.0f, window_h - TOOLBAR_H, color_border());

            em_widgets_end();
        }

        /* drawing */
        if (redraw) {

            /* agents */
            {
                vec4 bg = color_bg();
                graphics_viewport((int)SIDEBAR_W, (int)STATUSBAR_H, (int)(window_w - SIDEBAR_W), (int)(window_h - TOOLBAR_H - STATUSBAR_H));
                graphics_clear(bg.x, bg.y, bg.z);
                graphics_clear_depth();
                graphics_enable_depth_test(1);

                drawing_camera_setup(&model, (int)(window_w - SIDEBAR_W), (int)(window_h - TOOLBAR_H - STATUSBAR_H));

                for (unsigned group_i = 0; group_i < model.groups_count; ++group_i) {
                    EmGroup *group = model.groups + group_i;
                    drawing_draw_group(group, group_i);
                }
            }

            /* widgets */
            {
                mat4 projection;
                graphics_viewport(0, 0, window_w, window_h);
                graphics_enable_depth_test(0);
                graphics_ortho(&projection, 0.0f, (float)window_w, 0.0f, (float)window_h, -100.0f, 100.0f);

                em_widgets_draw(projection);
            }

            redraw = 0;

            glfwSwapBuffers(window);
        }

        glfwPollEvents();
    }

    ReleaseSemaphore(run_thread_semaphore, 1, 0);
    WaitForSingleObject(main_thread_semaphore, INFINITE);
    CloseHandle(main_thread_semaphore);
    CloseHandle(run_thread_semaphore);
    CloseHandle(run_thread);

    tay_simulation_end(tay);
    tay_threads_stop();
    tay_destroy_state(tay);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
