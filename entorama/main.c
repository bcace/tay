#include "main.h"
#include "entorama.h"
#include "shaders.h"
#include "tay.h"
#include "thread.h" // TODO: remove this!!!
#include "graphics.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


static int quit = 0;
static int paused = 1;
static int window_w = 1600;
static int window_h = 800;

static float view_pan_x = 0.0f;
static float view_pan_y = 0.0f;
static float view_rot_x = -1.0f;
static float view_rot_y = 0.0f;
static float view_zoom = 0.2f;

static int mouse_started_moving = 0;
static int mouse_l = 0;
static int mouse_r = 0;
static float mouse_x;
static float mouse_y;
static float mouse_dx = 0.0f;
static float mouse_dy = 0.0f;

typedef enum {
    CAMERA_MODELING,
    CAMERA_FLOATING,
} CameraType;

CameraType camera_type = CAMERA_MODELING;

static void _close_callback(GLFWwindow *window) {
    quit = 1;
}

static void _scroll_callback(GLFWwindow *glfw_window, double x, double y) {
    if (camera_type == CAMERA_MODELING) {
        if (y > 0.0) {
            view_zoom *= 0.95f;
            view_pan_x /= 0.95f;
            view_pan_y /= 0.95f;
        }
        else if (y < 0.0) {
            view_zoom /= 0.95f;
            view_pan_x *= 0.95f;
            view_pan_y *= 0.95f;
        }
    }
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

    if (camera_type == CAMERA_MODELING) {
        if (mouse_l) {
            view_rot_x -= mouse_dy * 0.001f;
            view_rot_y += mouse_dx * 0.001f;
        }
        else if (mouse_r) {
            view_pan_x -= mouse_dx;
            view_pan_y -= mouse_dy;
        }
    }
}

static void _key_callback(GLFWwindow *glfw_window, int key, int code, int action, int mods) {
    if (key == GLFW_KEY_Q && mods & GLFW_MOD_CONTROL)
        quit = 1;
    else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        paused = !paused;
}

static void _init_simulation_info(EntoramaSimulationInfo *info) {
    info->groups_count = 0;
    for (unsigned i = 0; i < ENTORAMA_MAX_GROUPS; ++i) {
        EntoramaGroupInfo *group = info->groups + i;
        group->group = 0;
        group->max_agents = 0;
        group->position_x_offset = 0;
        group->position_y_offset = 4;
        group->position_z_offset = 8;
        group->direction_source = ENTORAMA_DIRECTION_AUTO;
        group->color_source = ENTORAMA_COLOR_AUTO;
        group->size_source = ENTORAMA_SIZE_AUTO;
    }
}

static void _init_shader_program(Program *prog, const char *vert_defines) {
    shader_program_init(prog, agent_vert, "agent.vert", vert_defines, agent_frag, "agent.frag", "#version 450\n");
    shader_program_define_in_float(prog, 3);            /* vertex position */
    shader_program_define_instanced_in_float(prog, 3);  /* instance position */
    shader_program_define_instanced_in_float(prog, 3);  /* instance direction fwd */
    shader_program_define_instanced_in_float(prog, 4);  /* instance color */
    shader_program_define_instanced_in_float(prog, 3);  /* instance size */
    shader_program_define_uniform(prog, "projection");
    shader_program_define_uniform(prog, "modelview");
    shader_program_define_uniform(prog, "uniform_color");
    shader_program_define_uniform(prog, "uniform_size");
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

    Program program_basic;
    Program program_directed;
    Program program_color;
    Program program_color_directed;
    _init_shader_program(&program_basic, "#version 450\n");
    _init_shader_program(&program_directed, "#version 450\n#define ENTORAMA_DIRECTION_FWD\n");
    _init_shader_program(&program_color, "#version 450\n#define ENTORAMA_COLOR_AGENT\n");
    _init_shader_program(&program_color_directed, "#version 450\n#define ENTORAMA_COLOR_AGENT\n#define ENTORAMA_DIRECTION_FWD\n");

    EntoramaModelInfo model_info;
    model_info.init = 0;
    model_load(&model_info, "m_flocking.dll");

    TayState *tay = tay_create_state();

    EntoramaSimulationInfo sim_info;
    _init_simulation_info(&sim_info);
    model_info.init(&sim_info, tay);

    tay_threads_start(100000); // TODO: remove this!!!
    tay_simulation_start(tay);

    const int max_agents_count = 1000000; // TODO: adapt this to the actual inited model!

    vec3 *inst_pos = malloc(sizeof(vec3) * max_agents_count);
    vec3 *inst_dir_fwd = malloc(sizeof(vec3) * max_agents_count);
    vec4 *inst_color = malloc(sizeof(vec4) * max_agents_count);
    vec3 *inst_size = malloc(sizeof(vec3) * max_agents_count);

    float camera_fov;
    float camera_near;
    float camera_far;
    vec3 camera_pos;
    vec3 camera_fwd;
    vec3 camera_up;

    if (camera_type == CAMERA_MODELING) {

    }
    else {
        camera_fov = 1.2f;
        camera_near = 0.1f;

        camera_pos.x = model_info.origin_x;
        camera_pos.y = model_info.origin_y;
        camera_pos.z = model_info.origin_z + model_info.radius * 4.0f;

        camera_fwd.x = 0.0f;
        camera_fwd.y = 0.0f;
        camera_fwd.z = -1.0;

        camera_up.x = 0.0f;
        camera_up.y = 1.0f;
        camera_up.z = 0.0f;

        camera_far = model_info.radius * 6.0f;
    }

    Program *prev_prog = 0;

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

            mat4 projection;
            mat4 modelview;

            if (camera_type == CAMERA_MODELING) {

                graphics_frustum(&projection,
                     0.00001f * view_zoom * (view_pan_x - window_w * 0.5f),
                     0.00001f * view_zoom * (view_pan_x + window_w * 0.5f),
                     0.00001f * view_zoom * (view_pan_y - window_h * 0.5f),
                     0.00001f * view_zoom * (view_pan_y + window_h * 0.5f),
                     0.001f, 200.0f);

                mat4_set_identity(&modelview);
                mat4_translate(&modelview, 0.0f, 0.0f, -100.0f);
                mat4_rotate(&modelview, view_rot_x, 1.0f, 0.0f, 0.0f);
                mat4_rotate(&modelview, view_rot_y, 0.0f, 0.0f, 1.0f);
                mat4_scale(&modelview, 50.0f / model_info.radius);
                mat4_translate(&modelview, -model_info.origin_x, -model_info.origin_y, -model_info.origin_z);
            }
            else {
                mat4 perspective;
                graphics_perspective(&perspective, camera_fov, (float)window_w / (float)window_h, camera_near, camera_far);

                mat4 lookat;
                graphics_lookat(&lookat, camera_pos, camera_fwd, camera_up);

                mat4_multiply(&projection, &perspective, &lookat);

                mat4_set_identity(&modelview);
            }

            for (unsigned group_i = 0; group_i < sim_info.groups_count; ++group_i) {
                EntoramaGroupInfo *group_info = sim_info.groups + group_i;

                Program *prog;
                if (group_info->direction_source) {
                    if (group_info->color_source == ENTORAMA_COLOR_AGENT_PALETTE || group_info->color_source == ENTORAMA_COLOR_AGENT_RGB)
                        prog = &program_color_directed;
                    else
                        prog = &program_directed;
                }
                else {
                    if (group_info->color_source == ENTORAMA_COLOR_AGENT_PALETTE || group_info->color_source == ENTORAMA_COLOR_AGENT_RGB)
                        prog = &program_color;
                    else
                        prog = &program_basic;
                }

                if (prog != prev_prog) {
                    shader_program_use(prog);
                    prev_prog = prog;
                }

                shader_program_set_uniform_mat4(prog, 0, &projection);
                shader_program_set_uniform_mat4(prog, 1, &modelview);

                vec4 uniform_color;
                if (group_info->color_source == ENTORAMA_COLOR_UNIFORM_PALETTE)
                    uniform_color = color_palette(group_info->palette_index % 4);
                else if (group_info->color_source == ENTORAMA_COLOR_UNIFORM_RGB)
                    uniform_color = (vec4){group_info->red, group_info->green, group_info->blue, 1.0f};
                else
                    uniform_color = color_palette(group_i % 4);

                vec3 uniform_size = {1.0f, 1.0f, 1.0f};

                shader_program_set_uniform_vec4(prog, 2, &uniform_color);
                shader_program_set_uniform_vec3(prog, 3, &uniform_size);

                if (group_info->direction_source) {
                    if (group_info->color_source == ENTORAMA_COLOR_AGENT_PALETTE) {
                        for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                            char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                            inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                            inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                            inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                            inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                            inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                            inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            inst_color[agent_i] = color_palette(*(unsigned *)(data + group_info->color_palette_index_offset) % 4);
                        }
                    }
                    else {
                        for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                            char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                            inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                            inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                            inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                            inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                            inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                            inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                        }
                    }
                }
                else {
                    if (group_info->color_source == ENTORAMA_COLOR_AGENT_PALETTE) {
                        for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                            char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                            inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                            inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                            inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                            inst_color[agent_i] = color_palette(*(unsigned *)(data + group_info->color_palette_index_offset) % 4);
                        }
                    }
                    else {
                        for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                            char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                            inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                            inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                            inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                        }
                    }
                }

                shader_program_set_data_float(prog, 0, PYRAMID_VERTS_COUNT, 3, PYRAMID_VERTS);
                shader_program_set_data_float(prog, 1, group_info->max_agents, 3, inst_pos);
                shader_program_set_data_float(prog, 2, group_info->max_agents, 3, inst_dir_fwd);
                shader_program_set_data_float(prog, 3, group_info->max_agents, 4, inst_color);
                shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                graphics_draw_triangles_instanced(PYRAMID_VERTS_COUNT, group_info->max_agents);
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
