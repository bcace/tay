#include "main.h"
#include "tay.h"
#include "entorama.h"
#include "graphics.h"
#include "shaders.h"
#include <stdlib.h>


typedef enum {
    CAMERA_MODELING,
    CAMERA_FLOATING,
} CameraType;

typedef struct {
    CameraType type;
    union {
        struct {
            float pan_x;
            float pan_y;
            float rot_x;
            float rot_y;
            float zoom;
        };
        struct {
            float fov;
            float near;
            float far;
            vec3 pos;
            vec3 fwd;
            vec3 up;
        };
    };
    mat4 projection;
    mat4 modelview;
} Camera;

static Camera camera;
static Program program_basic;
static Program program_color;
static Program program_size;
static Program program_direction;
static Program program_color_size;
static Program program_direction_size;
static Program program_color_direction;
static Program program_color_direction_size;
static vec3 *inst_pos;
static vec3 *inst_dir_fwd;
static vec4 *inst_color;
static vec3 *inst_size;
static int SPHERE_VERTS_COUNT;
static float *SPHERE_VERTS;

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

void drawing_init(int max_agents_per_group) {
    camera.type = CAMERA_MODELING;

    camera.pan_x = 0.0f;
    camera.pan_y = 0.0f;
    camera.rot_x = -1.0f;
    camera.rot_y = 0.0f;
    camera.zoom = 0.2f;

    // camera.fov = 1.2f;
    // camera.near = 0.1f;
    // camera.pos.x = model_info.origin_x;
    // camera.pos.y = model_info.origin_y;
    // camera.pos.z = model_info.origin_z + model_info.radius * 4.0f;
    // camera.fwd.x = 0.0f;
    // camera.fwd.y = 0.0f;
    // camera.fwd.z = -1.0;
    // camera.up.x = 0.0f;
    // camera.up.y = 1.0f;
    // camera.up.z = 0.0f;
    // camera.far = model_info.radius * 6.0f;

    _init_shader_program(&program_basic, "#version 450\n");
    _init_shader_program(&program_color, "#version 450\n#define ENTORAMA_COLOR_AGENT\n");
    _init_shader_program(&program_size, "#version 450\n#define ENTORAMA_SIZE_AGENT\n");
    _init_shader_program(&program_direction, "#version 450\n#define ENTORAMA_DIRECTION_FWD\n");
    _init_shader_program(&program_color_size, "#version 450\n#define ENTORAMA_COLOR_AGENT\n#define ENTORAMA_SIZE_AGENT\n");
    _init_shader_program(&program_direction_size, "#version 450\n#define ENTORAMA_DIRECTION_FWD\n#define ENTORAMA_SIZE_AGENT\n");
    _init_shader_program(&program_color_direction, "#version 450\n#define ENTORAMA_COLOR_AGENT\n#define ENTORAMA_DIRECTION_FWD\n");
    _init_shader_program(&program_color_direction_size, "#version 450\n#define ENTORAMA_COLOR_AGENT\n#define ENTORAMA_DIRECTION_FWD\n#define ENTORAMA_SIZE_AGENT\n");

    inst_pos = malloc(sizeof(vec3) * max_agents_per_group);
    inst_dir_fwd = malloc(sizeof(vec3) * max_agents_per_group);
    inst_color = malloc(sizeof(vec4) * max_agents_per_group);
    inst_size = malloc(sizeof(vec3) * max_agents_per_group);

    const unsigned SPHERE_SUBDIVS = 1;
    SPHERE_VERTS_COUNT = icosahedron_verts_count(SPHERE_SUBDIVS);
    SPHERE_VERTS = malloc(SPHERE_VERTS_COUNT * 3 * sizeof(float));
    icosahedron_verts(SPHERE_SUBDIVS, SPHERE_VERTS);
}

void drawing_mouse_scroll(double y) {
    if (camera.type == CAMERA_MODELING) {
        if (y > 0.0) {
            camera.zoom *= 0.95f;
            camera.pan_x /= 0.95f;
            camera.pan_y /= 0.95f;
        }
        else if (y < 0.0) {
            camera.zoom /= 0.95f;
            camera.pan_x *= 0.95f;
            camera.pan_y *= 0.95f;
        }
    }
}

void drawing_mouse_move(int button_l, int button_r, float dx, float dy) {
    if (camera.type == CAMERA_MODELING) {
        if (button_l) {
            camera.rot_x -= dy * 0.001f;
            camera.rot_y += dx * 0.001f;
        }
        else if (button_r) {
            camera.pan_x -= dx;
            camera.pan_y -= dy;
        }
    }
}

void drawing_camera_setup(EntoramaModelInfo *info, int window_w, int window_h) {
    mat4_set_identity(&camera.modelview);
    if (camera.type == CAMERA_MODELING) {
        graphics_frustum(&camera.projection,
            0.00001f * camera.zoom * (camera.pan_x - window_w * 0.5f),
            0.00001f * camera.zoom * (camera.pan_x + window_w * 0.5f),
            0.00001f * camera.zoom * (camera.pan_y - window_h * 0.5f),
            0.00001f * camera.zoom * (camera.pan_y + window_h * 0.5f),
            0.001f, 200.0f
        );
        mat4_translate(&camera.modelview, 0.0f, 0.0f, -100.0f);
        mat4_rotate(&camera.modelview, camera.rot_x, 1.0f, 0.0f, 0.0f);
        mat4_rotate(&camera.modelview, camera.rot_y, 0.0f, 0.0f, 1.0f);
        mat4_scale(&camera.modelview, 50.0f / info->radius);
        mat4_translate(&camera.modelview, -info->origin_x, -info->origin_y, -info->origin_z);
    }
    else {
        mat4 perspective, lookat;
        graphics_perspective(&perspective, camera.fov, (float)window_w / (float)window_h, camera.near, camera.far);
        graphics_lookat(&lookat, camera.pos, camera.fwd, camera.up);
        mat4_multiply(&camera.projection, &perspective, &lookat);
        mat4_set_identity(&camera.modelview);
    }
}

void drawing_draw_group(TayState *tay, EntoramaGroupInfo *group, int group_i) {
    Program *prog = 0;

    /* select the agent drawing shader program */
    {
        if (group->direction_source) {
            if (group->color_source == ENTORAMA_COLOR_AGENT_PALETTE || group->color_source == ENTORAMA_COLOR_AGENT_RGB) {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS || group->size_source == ENTORAMA_SIZE_AGENT_XYZ)
                    prog = &program_color_direction_size;
                else
                    prog = &program_color_direction;
            }
            else {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS || group->size_source == ENTORAMA_SIZE_AGENT_XYZ)
                    prog = &program_direction_size;
                else
                    prog = &program_direction;
            }
        }
        else {
            if (group->color_source == ENTORAMA_COLOR_AGENT_PALETTE || group->color_source == ENTORAMA_COLOR_AGENT_RGB) {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS || group->size_source == ENTORAMA_SIZE_AGENT_XYZ)
                    prog = &program_color_size;
                else
                    prog = &program_color;
            }
            else {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS || group->size_source == ENTORAMA_SIZE_AGENT_XYZ)
                    prog = &program_size;
                else
                    prog = &program_basic;
            }
        }

        shader_program_use(prog);
        shader_program_set_uniform_mat4(prog, 0, &camera.projection);
        shader_program_set_uniform_mat4(prog, 1, &camera.modelview);
    }

    /* set color mode */
    {
        vec4 uniform_color;
        if (group->color_source == ENTORAMA_COLOR_UNIFORM_PALETTE)
            uniform_color = color_palette(group->palette_index % 4);
        else if (group->color_source == ENTORAMA_COLOR_UNIFORM_RGB)
            uniform_color = (vec4){group->red, group->green, group->blue, 1.0f};
        else
            uniform_color = color_palette(group_i % 4);

        vec3 uniform_size;
        if (group->size_source == ENTORAMA_SIZE_UNIFORM_RADIUS)
            uniform_size = (vec3){group->size_radius, group->size_radius, group->size_radius};
        else if (group->size_source == ENTORAMA_SIZE_UNIFORM_XYZ)
            uniform_size = (vec3){group->size_x, group->size_y, group->size_z};
        else
            uniform_size = (vec3){1.0f, 1.0f, 1.0f};

        shader_program_set_uniform_vec4(prog, 2, &uniform_color);
        shader_program_set_uniform_vec3(prog, 3, &uniform_size);
    }

    /* push agent data */
    {
        if (group->direction_source) {
            if (group->color_source == ENTORAMA_COLOR_AGENT_PALETTE) {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i] = color_palette(*(unsigned *)(data + group->color_palette_index_offset) % 4);
                        float radius = *(float *)(data + group->size_radius_offset);
                        inst_size[agent_i] = (vec3){radius, radius, radius};
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else if (group->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i] = color_palette(*(unsigned *)(data + group->color_palette_index_offset) % 4);
                        inst_size[agent_i].x = *(float *)(data + group->size_x_offset);
                        inst_size[agent_i].y = *(float *)(data + group->size_y_offset);
                        inst_size[agent_i].z = *(float *)(data + group->size_z_offset);
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i] = color_palette(*(unsigned *)(data + group->color_palette_index_offset) % 4);
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }
                }

                shader_program_set_data_float(prog, 3, group->max_agents, 4, inst_color);
            }
            else if (group->color_source == ENTORAMA_COLOR_AGENT_RGB) {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i].x = *(float *)(data + group->color_r_offset);
                        inst_color[agent_i].y = *(float *)(data + group->color_g_offset);
                        inst_color[agent_i].z = *(float *)(data + group->color_b_offset);
                        inst_color[agent_i].w = 1.0f;
                        float radius = *(float *)(data + group->size_radius_offset);
                        inst_size[agent_i] = (vec3){radius, radius, radius};
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else if (group->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i].x = *(float *)(data + group->color_r_offset);
                        inst_color[agent_i].y = *(float *)(data + group->color_g_offset);
                        inst_color[agent_i].z = *(float *)(data + group->color_b_offset);
                        inst_color[agent_i].w = 1.0f;
                        inst_size[agent_i].x = *(float *)(data + group->size_x_offset);
                        inst_size[agent_i].y = *(float *)(data + group->size_y_offset);
                        inst_size[agent_i].z = *(float *)(data + group->size_z_offset);
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i].x = *(float *)(data + group->color_r_offset);
                        inst_color[agent_i].y = *(float *)(data + group->color_g_offset);
                        inst_color[agent_i].z = *(float *)(data + group->color_b_offset);
                        inst_color[agent_i].w = 1.0f;
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }
                }

                shader_program_set_data_float(prog, 3, group->max_agents, 4, inst_color);
            }
            else {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        float radius = *(float *)(data + group->size_radius_offset);
                        inst_size[agent_i] = (vec3){radius, radius, radius};
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else if (group->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_size[agent_i].x = *(float *)(data + group->size_x_offset);
                        inst_size[agent_i].y = *(float *)(data + group->size_y_offset);
                        inst_size[agent_i].z = *(float *)(data + group->size_z_offset);
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_dir_fwd[agent_i].x = *(float *)(data + group->direction_fwd_x_offset);
                        inst_dir_fwd[agent_i].y = *(float *)(data + group->direction_fwd_y_offset);
                        inst_dir_fwd[agent_i].z = *(float *)(data + group->direction_fwd_z_offset);
                    }
                }
            }

            shader_program_set_data_float(prog, 2, group->max_agents, 3, inst_dir_fwd);
        }
        else {
            if (group->color_source == ENTORAMA_COLOR_AGENT_PALETTE) {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i] = color_palette(*(unsigned *)(data + group->color_palette_index_offset) % 4);
                        float radius = *(float *)(data + group->size_radius_offset);
                        inst_size[agent_i] = (vec3){radius, radius, radius};
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else if (group->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i] = color_palette(*(unsigned *)(data + group->color_palette_index_offset) % 4);
                        inst_size[agent_i].x = *(float *)(data + group->size_x_offset);
                        inst_size[agent_i].y = *(float *)(data + group->size_y_offset);
                        inst_size[agent_i].z = *(float *)(data + group->size_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i] = color_palette(*(unsigned *)(data + group->color_palette_index_offset) % 4);
                    }
                }

                shader_program_set_data_float(prog, 3, group->max_agents, 4, inst_color);
            }
            else if (group->color_source == ENTORAMA_COLOR_AGENT_RGB) {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i].x = *(float *)(data + group->color_r_offset);
                        inst_color[agent_i].y = *(float *)(data + group->color_g_offset);
                        inst_color[agent_i].z = *(float *)(data + group->color_b_offset);
                        inst_color[agent_i].w = 1.0f;
                        float radius = *(float *)(data + group->size_radius_offset);
                        inst_size[agent_i] = (vec3){radius, radius, radius};
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else if (group->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i].x = *(float *)(data + group->color_r_offset);
                        inst_color[agent_i].y = *(float *)(data + group->color_g_offset);
                        inst_color[agent_i].z = *(float *)(data + group->color_b_offset);
                        inst_color[agent_i].w = 1.0f;
                        inst_size[agent_i].x = *(float *)(data + group->size_x_offset);
                        inst_size[agent_i].y = *(float *)(data + group->size_y_offset);
                        inst_size[agent_i].z = *(float *)(data + group->size_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i].x = *(float *)(data + group->color_r_offset);
                        inst_color[agent_i].y = *(float *)(data + group->color_g_offset);
                        inst_color[agent_i].z = *(float *)(data + group->color_b_offset);
                        inst_color[agent_i].w = 1.0f;
                    }
                }

                shader_program_set_data_float(prog, 3, group->max_agents, 4, inst_color);
            }
            else {
                if (group->size_source == ENTORAMA_SIZE_AGENT_RADIUS) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        float radius = *(float *)(data + group->size_radius_offset);
                        inst_size[agent_i] = (vec3){radius, radius, radius};
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else if (group->size_source == ENTORAMA_SIZE_AGENT_XYZ) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_size[agent_i].x = *(float *)(data + group->size_x_offset);
                        inst_size[agent_i].y = *(float *)(data + group->size_y_offset);
                        inst_size[agent_i].z = *(float *)(data + group->size_z_offset);
                    }

                    shader_program_set_data_float(prog, 4, group->max_agents, 3, inst_size);
                }
                else {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                    }
                }
            }
        }

        shader_program_set_data_float(prog, 1, group->max_agents, 3, inst_pos);
    }

    /* push agent model geometry */
    {
        int verts_count = 0;
        float *verts = 0;
        if (group->shape == ENTORAMA_CUBE) {
            verts_count = CUBE_VERTS_COUNT;
            verts = CUBE_VERTS;
        }
        else if (group->shape == ENTORAMA_PYRAMID) {
            verts_count = PYRAMID_VERTS_COUNT;
            verts = PYRAMID_VERTS;
        }
        else {
            verts_count = SPHERE_VERTS_COUNT;
            verts = SPHERE_VERTS;
        }
        shader_program_set_data_float(prog, 0, verts_count, 3, verts);
        graphics_draw_triangles_instanced(verts_count, group->max_agents);
    }
}
