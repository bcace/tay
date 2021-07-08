#include "main.h"
#include "entorama.h"
#include "shaders.h"
#include "tay.h"
#include "thread.h" // TODO: remove this!!!
#include "graphics.h"
#include "font.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


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
        group->shape = ENTORAMA_CUBE;
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

    Program program_basic;
    Program program_color;
    Program program_size;
    Program program_direction;
    Program program_color_size;
    Program program_direction_size;
    Program program_color_direction;
    Program program_color_direction_size;
    _init_shader_program(&program_basic, "#version 450\n");
    _init_shader_program(&program_color, "#version 450\n#define ENTORAMA_COLOR_AGENT\n");
    _init_shader_program(&program_size, "#version 450\n#define ENTORAMA_SIZE_AGENT\n");
    _init_shader_program(&program_direction, "#version 450\n#define ENTORAMA_DIRECTION_FWD\n");
    _init_shader_program(&program_color_size, "#version 450\n#define ENTORAMA_COLOR_AGENT\n#define ENTORAMA_SIZE_AGENT\n");
    _init_shader_program(&program_direction_size, "#version 450\n#define ENTORAMA_DIRECTION_FWD\n#define ENTORAMA_SIZE_AGENT\n");
    _init_shader_program(&program_color_direction, "#version 450\n#define ENTORAMA_COLOR_AGENT\n#define ENTORAMA_DIRECTION_FWD\n");
    _init_shader_program(&program_color_direction_size, "#version 450\n#define ENTORAMA_COLOR_AGENT\n#define ENTORAMA_DIRECTION_FWD\n#define ENTORAMA_SIZE_AGENT\n");

    font_init();

    EntoramaModelInfo model_info;
    model_info.init = 0;
    model_load(&model_info, "m_sph.dll");

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

    drawing_init();

    const unsigned SPHERE_SUBDIVS = 1;
    int SPHERE_VERTS_COUNT = icosahedron_verts_count(SPHERE_SUBDIVS);
    float *SPHERE_VERTS = malloc(SPHERE_VERTS_COUNT * 3 * sizeof(float));
    icosahedron_verts(SPHERE_SUBDIVS, SPHERE_VERTS);

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

            drawing_camera_setup(&model_info, window_w, window_h, &projection, &modelview);

            for (unsigned group_i = 0; group_i < sim_info.groups_count; ++group_i) {
                EntoramaGroupInfo *group_info = sim_info.groups + group_i;

                Program *prog;
                if (group_info->direction_source) {
                    if (group_info->color_source == ENTORAMA_COLOR_AGENT_PALETTE || group_info->color_source == ENTORAMA_COLOR_AGENT_RGB) {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS || group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ)
                            prog = &program_color_direction_size;
                        else
                            prog = &program_color_direction;
                    }
                    else {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS || group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ)
                            prog = &program_direction_size;
                        else
                            prog = &program_direction;
                    }
                }
                else {
                    if (group_info->color_source == ENTORAMA_COLOR_AGENT_PALETTE || group_info->color_source == ENTORAMA_COLOR_AGENT_RGB) {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS || group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ)
                            prog = &program_color_size;
                        else
                            prog = &program_color;
                    }
                    else {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS || group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ)
                            prog = &program_size;
                        else
                            prog = &program_basic;
                    }
                }

                shader_program_use(prog);

                shader_program_set_uniform_mat4(prog, 0, &projection);
                shader_program_set_uniform_mat4(prog, 1, &modelview);

                vec4 uniform_color;
                if (group_info->color_source == ENTORAMA_COLOR_UNIFORM_PALETTE)
                    uniform_color = color_palette(group_info->palette_index % 4);
                else if (group_info->color_source == ENTORAMA_COLOR_UNIFORM_RGB)
                    uniform_color = (vec4){group_info->red, group_info->green, group_info->blue, 1.0f};
                else
                    uniform_color = color_palette(group_i % 4);

                vec3 uniform_size;
                if (group_info->size_source == ENTORAMA_SIZE_UNIFORM_RADIUS)
                    uniform_size = (vec3){group_info->size_radius, group_info->size_radius, group_info->size_radius};
                else if (group_info->size_source == ENTORAMA_SIZE_UNIFORM_XYZ)
                    uniform_size = (vec3){group_info->size_x, group_info->size_y, group_info->size_z};
                else
                    uniform_size = (vec3){1.0f, 1.0f, 1.0f};

                shader_program_set_uniform_vec4(prog, 2, &uniform_color);
                shader_program_set_uniform_vec3(prog, 3, &uniform_size);

                if (group_info->direction_source) {
                    if (group_info->color_source == ENTORAMA_COLOR_AGENT_PALETTE) {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i] = color_palette(*(unsigned *)(data + group_info->color_palette_index_offset) % 4);
                                float radius = *(float *)(data + group_info->size_radius_offset);
                                inst_size[agent_i] = (vec3){radius, radius, radius};
                                inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                                inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                                inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else if (group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i] = color_palette(*(unsigned *)(data + group_info->color_palette_index_offset) % 4);
                                inst_size[agent_i].x = *(float *)(data + group_info->size_x_offset);
                                inst_size[agent_i].y = *(float *)(data + group_info->size_y_offset);
                                inst_size[agent_i].z = *(float *)(data + group_info->size_z_offset);
                                inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                                inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                                inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i] = color_palette(*(unsigned *)(data + group_info->color_palette_index_offset) % 4);
                                inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                                inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                                inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            }
                        }

                        shader_program_set_data_float(prog, 3, group_info->max_agents, 4, inst_color);
                    }
                    else if (group_info->color_source == ENTORAMA_COLOR_AGENT_RGB) {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i].x = *(float *)(data + group_info->color_r_offset);
                                inst_color[agent_i].y = *(float *)(data + group_info->color_g_offset);
                                inst_color[agent_i].z = *(float *)(data + group_info->color_b_offset);
                                inst_color[agent_i].w = 1.0f;
                                float radius = *(float *)(data + group_info->size_radius_offset);
                                inst_size[agent_i] = (vec3){radius, radius, radius};
                                inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                                inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                                inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else if (group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i].x = *(float *)(data + group_info->color_r_offset);
                                inst_color[agent_i].y = *(float *)(data + group_info->color_g_offset);
                                inst_color[agent_i].z = *(float *)(data + group_info->color_b_offset);
                                inst_color[agent_i].w = 1.0f;
                                inst_size[agent_i].x = *(float *)(data + group_info->size_x_offset);
                                inst_size[agent_i].y = *(float *)(data + group_info->size_y_offset);
                                inst_size[agent_i].z = *(float *)(data + group_info->size_z_offset);
                                inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                                inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                                inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i].x = *(float *)(data + group_info->color_r_offset);
                                inst_color[agent_i].y = *(float *)(data + group_info->color_g_offset);
                                inst_color[agent_i].z = *(float *)(data + group_info->color_b_offset);
                                inst_color[agent_i].w = 1.0f;
                                inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                                inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                                inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            }
                        }

                        shader_program_set_data_float(prog, 3, group_info->max_agents, 4, inst_color);
                    }
                    else {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                float radius = *(float *)(data + group_info->size_radius_offset);
                                inst_size[agent_i] = (vec3){radius, radius, radius};
                                inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                                inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                                inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else if (group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_size[agent_i].x = *(float *)(data + group_info->size_x_offset);
                                inst_size[agent_i].y = *(float *)(data + group_info->size_y_offset);
                                inst_size[agent_i].z = *(float *)(data + group_info->size_z_offset);
                                inst_dir_fwd[agent_i].x = *(float *)(data + group_info->direction_fwd_x_offset);
                                inst_dir_fwd[agent_i].y = *(float *)(data + group_info->direction_fwd_y_offset);
                                inst_dir_fwd[agent_i].z = *(float *)(data + group_info->direction_fwd_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
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

                    shader_program_set_data_float(prog, 2, group_info->max_agents, 3, inst_dir_fwd);
                }
                else {
                    if (group_info->color_source == ENTORAMA_COLOR_AGENT_PALETTE) {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i] = color_palette(*(unsigned *)(data + group_info->color_palette_index_offset) % 4);
                                float radius = *(float *)(data + group_info->size_radius_offset);
                                inst_size[agent_i] = (vec3){radius, radius, radius};
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else if (group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i] = color_palette(*(unsigned *)(data + group_info->color_palette_index_offset) % 4);
                                inst_size[agent_i].x = *(float *)(data + group_info->size_x_offset);
                                inst_size[agent_i].y = *(float *)(data + group_info->size_y_offset);
                                inst_size[agent_i].z = *(float *)(data + group_info->size_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i] = color_palette(*(unsigned *)(data + group_info->color_palette_index_offset) % 4);
                            }
                        }

                        shader_program_set_data_float(prog, 3, group_info->max_agents, 4, inst_color);
                    }
                    else if (group_info->color_source == ENTORAMA_COLOR_AGENT_RGB) {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i].x = *(float *)(data + group_info->color_r_offset);
                                inst_color[agent_i].y = *(float *)(data + group_info->color_g_offset);
                                inst_color[agent_i].z = *(float *)(data + group_info->color_b_offset);
                                inst_color[agent_i].w = 1.0f;
                                float radius = *(float *)(data + group_info->size_radius_offset);
                                inst_size[agent_i] = (vec3){radius, radius, radius};
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else if (group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i].x = *(float *)(data + group_info->color_r_offset);
                                inst_color[agent_i].y = *(float *)(data + group_info->color_g_offset);
                                inst_color[agent_i].z = *(float *)(data + group_info->color_b_offset);
                                inst_color[agent_i].w = 1.0f;
                                inst_size[agent_i].x = *(float *)(data + group_info->size_x_offset);
                                inst_size[agent_i].y = *(float *)(data + group_info->size_y_offset);
                                inst_size[agent_i].z = *(float *)(data + group_info->size_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_color[agent_i].x = *(float *)(data + group_info->color_r_offset);
                                inst_color[agent_i].y = *(float *)(data + group_info->color_g_offset);
                                inst_color[agent_i].z = *(float *)(data + group_info->color_b_offset);
                                inst_color[agent_i].w = 1.0f;
                            }
                        }

                        shader_program_set_data_float(prog, 3, group_info->max_agents, 4, inst_color);
                    }
                    else {
                        if (group_info->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                float radius = *(float *)(data + group_info->size_radius_offset);
                                inst_size[agent_i] = (vec3){radius, radius, radius};
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
                        }
                        else if (group_info->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                            for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                                char *data = (char *)tay_get_agent(tay, group_info->group, agent_i) + sizeof(TayAgentTag);
                                inst_pos[agent_i].x = *(float *)(data + group_info->position_x_offset);
                                inst_pos[agent_i].y = *(float *)(data + group_info->position_y_offset);
                                inst_pos[agent_i].z = *(float *)(data + group_info->position_z_offset);
                                inst_size[agent_i].x = *(float *)(data + group_info->size_x_offset);
                                inst_size[agent_i].y = *(float *)(data + group_info->size_y_offset);
                                inst_size[agent_i].z = *(float *)(data + group_info->size_z_offset);
                            }

                            shader_program_set_data_float(prog, 4, group_info->max_agents, 3, inst_size);
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
                }

                shader_program_set_data_float(prog, 1, group_info->max_agents, 3, inst_pos);

                int verts_count = 0;
                float *verts = 0;
                if (group_info->shape == ENTORAMA_CUBE) {
                    verts_count = CUBE_VERTS_COUNT;
                    verts = CUBE_VERTS;
                }
                else if (group_info->shape == ENTORAMA_PYRAMID) {
                    verts_count = PYRAMID_VERTS_COUNT;
                    verts = PYRAMID_VERTS;
                }
                else {
                    verts_count = SPHERE_VERTS_COUNT;
                    verts = SPHERE_VERTS;
                }
                shader_program_set_data_float(prog, 0, verts_count, 3, verts);
                graphics_draw_triangles_instanced(verts_count, group_info->max_agents);
            }

            /* flat overlay */
            {
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
