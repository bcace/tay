#include "main.h"
#include "tay.h"
#include "entorama.h"
#include "graphics.h"
#include "shaders.h"
#include <stdlib.h>
#include <math.h>


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
static Program program_tools;
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

static vec3 world_box_verts[24];
static vec4 world_box_colors[24];
static int world_box_verts_count = 24;
static unsigned world_box_pos_buffer;
static unsigned world_box_col_buffer;

static void _init_shader_program(Program *prog, const char *vert_defines) {
    shader_program_init(prog, agent_vert, "agent.vert", vert_defines, agent_frag, "agent.frag", "#version 450\n");
    shader_program_define_uniform(prog, "projection");
    shader_program_define_uniform(prog, "modelview");
    shader_program_define_uniform(prog, "uniform_color");
    shader_program_define_uniform(prog, "uniform_size");
}

void drawing_init(int max_agents_per_group) {
    camera.type = CAMERA_MODELING;

    camera.pan_x = 0.0f;
    camera.pan_y = 0.0f;
    camera.rot_x = -1.2f;
    camera.rot_y = 1.0f;
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
    _init_shader_program(&program_color, "#version 450\n#define EM_COLOR_AGENT\n");
    _init_shader_program(&program_size, "#version 450\n#define EM_SIZE_AGENT\n");
    _init_shader_program(&program_direction, "#version 450\n#define EM_DIRECTION_FWD\n");
    _init_shader_program(&program_color_size, "#version 450\n#define EM_COLOR_AGENT\n#define EM_SIZE_AGENT\n");
    _init_shader_program(&program_direction_size, "#version 450\n#define EM_DIRECTION_FWD\n#define EM_SIZE_AGENT\n");
    _init_shader_program(&program_color_direction, "#version 450\n#define EM_COLOR_AGENT\n#define EM_DIRECTION_FWD\n");
    _init_shader_program(&program_color_direction_size, "#version 450\n#define EM_COLOR_AGENT\n#define EM_DIRECTION_FWD\n#define EM_SIZE_AGENT\n");

    shader_program_init(&program_tools, tools_vert, "tools.vert", "", tools_frag, "tools.frag", "");
    world_box_pos_buffer = graphics_create_buffer(0, world_box_verts_count, 3);
    world_box_col_buffer = graphics_create_buffer(1, world_box_verts_count, 4);
    shader_program_define_uniform(&program_tools, "projection");
    shader_program_define_uniform(&program_tools, "modelview");

    inst_pos = malloc(sizeof(vec3) * max_agents_per_group);
    inst_dir_fwd = malloc(sizeof(vec3) * max_agents_per_group);
    inst_color = malloc(sizeof(vec4) * max_agents_per_group);
    inst_size = malloc(sizeof(vec3) * max_agents_per_group);

    const unsigned SPHERE_SUBDIVS = 2;
    SPHERE_VERTS_COUNT = icosahedron_verts_count(SPHERE_SUBDIVS);
    SPHERE_VERTS = malloc(SPHERE_VERTS_COUNT * 3 * sizeof(float));
    icosahedron_verts(SPHERE_SUBDIVS, SPHERE_VERTS);
}

void drawing_mouse_scroll(double y, int *redraw) {
    if (camera.type == CAMERA_MODELING) {
        if (y > 0.0) {
            camera.zoom *= 0.95f;
            camera.pan_x /= 0.95f;
            camera.pan_y /= 0.95f;
            *redraw = 1;
        }
        else if (y < 0.0) {
            camera.zoom /= 0.95f;
            camera.pan_x *= 0.95f;
            camera.pan_y *= 0.95f;
            *redraw = 1;
        }
    }
}

void drawing_mouse_move(int button_l, int button_r, float dx, float dy, int *redraw) {
    if (camera.type == CAMERA_MODELING) {
        if (button_l) {
            camera.rot_x -= dy * 0.002f;
            camera.rot_y += dx * 0.002f;
            *redraw = 1;
        }
        else if (button_r) {
            camera.pan_x -= dx;
            camera.pan_y -= dy;
            *redraw = 1;
        }
    }
}

void drawing_camera_setup(EmModel *model, int window_w, int window_h) {
    mat4_set_identity(&camera.modelview);
    if (camera.type == CAMERA_MODELING) {
        graphics_frustum(&camera.projection,
            0.007f * camera.zoom * (camera.pan_x - window_w * 0.5f),
            0.007f * camera.zoom * (camera.pan_x + window_w * 0.5f),
            0.007f * camera.zoom * (camera.pan_y - window_h * 0.5f),
            0.007f * camera.zoom * (camera.pan_y + window_h * 0.5f),
            1.0f, 200.0f
        );
        mat4_translate(&camera.modelview, 0.0f, 0.0f, -100.0f);
        mat4_rotate(&camera.modelview, camera.rot_x, 1.0f, 0.0f, 0.0f);
        mat4_rotate(&camera.modelview, camera.rot_y, 0.0f, 0.0f, 1.0f);
        float dx = model->max_x - model->min_x;
        float dy = model->max_y - model->min_y;
        float dz = model->max_z - model->min_z;
        float d = sqrtf(dx * dx + dy * dy + dz * dz);
        mat4_scale(&camera.modelview, 100.0f / d);
        mat4_translate(&camera.modelview,
            (model->max_x + model->min_x) * -0.5f,
            (model->max_y + model->min_y) * -0.5f,
            (model->max_z + model->min_z) * -0.5f
        );
    }
    else {
        mat4 perspective, lookat;
        graphics_perspective(&perspective, camera.fov, (float)window_w / (float)window_h, camera.near, camera.far);
        graphics_lookat(&lookat, camera.pos, camera.fwd, camera.up);
        mat4_multiply(&camera.projection, &perspective, &lookat);
        mat4_set_identity(&camera.modelview);
    }
}

void drawing_update_world_box(EmModel *model) {
    world_box_verts[0] = (vec3){model->min_x, model->min_y, model->min_z};
    world_box_verts[1] = (vec3){model->max_x, model->min_y, model->min_z};
    world_box_verts[2] = (vec3){model->max_x, model->min_y, model->min_z};
    world_box_verts[3] = (vec3){model->max_x, model->max_y, model->min_z};
    world_box_verts[4] = (vec3){model->max_x, model->max_y, model->min_z};
    world_box_verts[5] = (vec3){model->min_x, model->max_y, model->min_z};
    world_box_verts[6] = (vec3){model->min_x, model->max_y, model->min_z};
    world_box_verts[7] = (vec3){model->min_x, model->min_y, model->min_z};
    world_box_verts[8] = (vec3){model->min_x, model->min_y, model->max_z};
    world_box_verts[9] = (vec3){model->max_x, model->min_y, model->max_z};
    world_box_verts[10] = (vec3){model->max_x, model->min_y, model->max_z};
    world_box_verts[11] = (vec3){model->max_x, model->max_y, model->max_z};
    world_box_verts[12] = (vec3){model->max_x, model->max_y, model->max_z};
    world_box_verts[13] = (vec3){model->min_x, model->max_y, model->max_z};
    world_box_verts[14] = (vec3){model->min_x, model->max_y, model->max_z};
    world_box_verts[15] = (vec3){model->min_x, model->min_y, model->max_z};
    world_box_verts[16] = (vec3){model->min_x, model->min_y, model->min_z};
    world_box_verts[17] = (vec3){model->min_x, model->min_y, model->max_z};
    world_box_verts[18] = (vec3){model->max_x, model->min_y, model->min_z};
    world_box_verts[19] = (vec3){model->max_x, model->min_y, model->max_z};
    world_box_verts[20] = (vec3){model->max_x, model->max_y, model->min_z};
    world_box_verts[21] = (vec3){model->max_x, model->max_y, model->max_z};
    world_box_verts[22] = (vec3){model->min_x, model->max_y, model->min_z};
    world_box_verts[23] = (vec3){model->min_x, model->max_y, model->max_z};

    vec4 color = color_fg();
    color.w = 0.1f;
    for (int i = 0; i < world_box_verts_count; ++i)
        world_box_colors[i] = color;
}

// TODO: do buffer cleanup on repeated calls
void drawing_init_group_drawing(EmGroup *group) {

    if (group->direction_source) {
        if (group->color_source == EM_COLOR_AGENT_PALETTE || group->color_source == EM_COLOR_AGENT_RGB) {
            if (group->size_source == EM_SIZE_AGENT_RADIUS || group->size_source == EM_SIZE_AGENT_XYZ) {
                group->program = &program_color_direction_size;
                shader_program_use(group->program);
                group->vert_buffer = graphics_create_buffer(0, 5000, 3);
                group->pos_buffer = graphics_create_buffer_instanced(1, group->max_agents, 3);
                group->dir_fwd_buffer = graphics_create_buffer_instanced(2, group->max_agents, 3);
                group->color_buffer = graphics_create_buffer_instanced(3, group->max_agents, 4);
                group->size_buffer = graphics_create_buffer_instanced(4, group->max_agents, 3);
            }
            else {
                group->program = &program_color_direction;
                shader_program_use(group->program);
                group->vert_buffer = graphics_create_buffer(0, 5000, 3);
                group->pos_buffer = graphics_create_buffer_instanced(1, group->max_agents, 3);
                group->dir_fwd_buffer = graphics_create_buffer_instanced(2, group->max_agents, 3);
                group->color_buffer = graphics_create_buffer_instanced(3, group->max_agents, 4);
            }
        }
        else {
            if (group->size_source == EM_SIZE_AGENT_RADIUS || group->size_source == EM_SIZE_AGENT_XYZ) {
                group->program = &program_direction_size;
                shader_program_use(group->program);
                group->vert_buffer = graphics_create_buffer(0, 5000, 3);
                group->pos_buffer = graphics_create_buffer_instanced(1, group->max_agents, 3);
                group->dir_fwd_buffer = graphics_create_buffer_instanced(2, group->max_agents, 3);
                group->size_buffer = graphics_create_buffer_instanced(4, group->max_agents, 3);
            }
            else {
                group->program = &program_direction;
                shader_program_use(group->program);
                group->vert_buffer = graphics_create_buffer(0, 5000, 3);
                group->pos_buffer = graphics_create_buffer_instanced(1, group->max_agents, 3);
                group->dir_fwd_buffer = graphics_create_buffer_instanced(2, group->max_agents, 3);
            }
        }
    }
    else {
        if (group->color_source == EM_COLOR_AGENT_PALETTE || group->color_source == EM_COLOR_AGENT_RGB) {
            if (group->size_source == EM_SIZE_AGENT_RADIUS || group->size_source == EM_SIZE_AGENT_XYZ) {
                group->program = &program_color_size;
                shader_program_use(group->program);
                group->vert_buffer = graphics_create_buffer(0, 5000, 3);
                group->pos_buffer = graphics_create_buffer_instanced(1, group->max_agents, 3);
                group->color_buffer = graphics_create_buffer_instanced(3, group->max_agents, 4);
                group->size_buffer = graphics_create_buffer_instanced(4, group->max_agents, 3);
            }
            else {
                group->program = &program_color;
                shader_program_use(group->program);
                group->vert_buffer = graphics_create_buffer(0, 5000, 3);
                group->pos_buffer = graphics_create_buffer_instanced(1, group->max_agents, 3);
                group->color_buffer = graphics_create_buffer_instanced(3, group->max_agents, 4);
            }
        }
        else {
            if (group->size_source == EM_SIZE_AGENT_RADIUS || group->size_source == EM_SIZE_AGENT_XYZ) {
                group->program = &program_size;
                shader_program_use(group->program);
                group->vert_buffer = graphics_create_buffer(0, 5000, 3);
                group->pos_buffer = graphics_create_buffer_instanced(1, group->max_agents, 3);
                group->size_buffer = graphics_create_buffer_instanced(4, group->max_agents, 3);
            }
            else {
                group->program = &program_basic;
                shader_program_use(group->program);
                group->vert_buffer = graphics_create_buffer(0, 5000, 3);
                group->pos_buffer = graphics_create_buffer_instanced(1, group->max_agents, 3);
            }
        }
    }
}

void drawing_draw_group(TayState *tay, EmGroup *group, int group_i) {
    Program *prog = 0;

    /* select the agent drawing shader program */
    {
        prog = group->program;
        shader_program_use(prog);
        shader_program_set_uniform_mat4(prog, 0, &camera.projection);
        shader_program_set_uniform_mat4(prog, 1, &camera.modelview);
    }

    /* set color mode */
    {
        vec4 uniform_color;
        if (group->color_source == EM_COLOR_UNIFORM_PALETTE)
            uniform_color = color_palette(group->palette_index % 4);
        else if (group->color_source == EM_COLOR_UNIFORM_RGB)
            uniform_color = (vec4){group->red, group->green, group->blue, 1.0f};
        else
            uniform_color = color_palette(group_i % 4);

        vec3 uniform_size;
        if (group->size_source == EM_SIZE_UNIFORM_RADIUS)
            uniform_size = (vec3){group->size_radius, group->size_radius, group->size_radius};
        else if (group->size_source == EM_SIZE_UNIFORM_XYZ)
            uniform_size = (vec3){group->size_x, group->size_y, group->size_z};
        else
            uniform_size = (vec3){1.0f, 1.0f, 1.0f};

        shader_program_set_uniform_vec4(prog, 2, &uniform_color);
        shader_program_set_uniform_vec3(prog, 3, &uniform_size);
    }

    /* push agent data */
    {
        if (group->direction_source) {
            if (group->color_source == EM_COLOR_AGENT_PALETTE) {
                if (group->size_source == EM_SIZE_AGENT_RADIUS) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
                }
                else if (group->size_source == EM_SIZE_AGENT_XYZ) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
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

                graphics_copy_to_buffer(group->color_buffer, inst_color, group->max_agents, 4);
            }
            else if (group->color_source == EM_COLOR_AGENT_RGB) {
                if (group->size_source == EM_SIZE_AGENT_RADIUS) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
                }
                else if (group->size_source == EM_SIZE_AGENT_XYZ) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
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

                graphics_copy_to_buffer(group->color_buffer, inst_color, group->max_agents, 4);
            }
            else {
                if (group->size_source == EM_SIZE_AGENT_RADIUS) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
                }
                else if (group->size_source == EM_SIZE_AGENT_XYZ) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
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

            graphics_copy_to_buffer(group->dir_fwd_buffer, inst_dir_fwd, group->max_agents, 3);
        }
        else {
            if (group->color_source == EM_COLOR_AGENT_PALETTE) {
                if (group->size_source == EM_SIZE_AGENT_RADIUS) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_color[agent_i] = color_palette(*(unsigned *)(data + group->color_palette_index_offset) % 4);
                        float radius = *(float *)(data + group->size_radius_offset);
                        inst_size[agent_i] = (vec3){radius, radius, radius};
                    }

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
                }
                else if (group->size_source == EM_SIZE_AGENT_XYZ) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
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

                graphics_copy_to_buffer(group->color_buffer, inst_color, group->max_agents, 4);
            }
            else if (group->color_source == EM_COLOR_AGENT_RGB) {
                if (group->size_source == EM_SIZE_AGENT_RADIUS) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
                }
                else if (group->size_source == EM_SIZE_AGENT_XYZ) {
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

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
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

                graphics_copy_to_buffer(group->color_buffer, inst_color, group->max_agents, 4);
            }
            else {
                if (group->size_source == EM_SIZE_AGENT_RADIUS) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        float radius = *(float *)(data + group->size_radius_offset);
                        inst_size[agent_i] = (vec3){radius, radius, radius};
                    }

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
                }
                else if (group->size_source == EM_SIZE_AGENT_XYZ) {
                    for (unsigned agent_i = 0; agent_i < group->max_agents; ++agent_i) {
                        char *data = (char *)tay_get_agent(tay, group->group, agent_i) + sizeof(TayAgentTag);
                        inst_pos[agent_i].x = *(float *)(data + group->position_x_offset);
                        inst_pos[agent_i].y = *(float *)(data + group->position_y_offset);
                        inst_pos[agent_i].z = *(float *)(data + group->position_z_offset);
                        inst_size[agent_i].x = *(float *)(data + group->size_x_offset);
                        inst_size[agent_i].y = *(float *)(data + group->size_y_offset);
                        inst_size[agent_i].z = *(float *)(data + group->size_z_offset);
                    }

                    graphics_copy_to_buffer(group->size_buffer, inst_size, group->max_agents, 3);
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

        graphics_copy_to_buffer(group->pos_buffer, inst_pos, group->max_agents, 3);
    }

    int verts_count = 0;

    /* push agent model geometry */
    {
        float *verts = 0;
        if (group->shape == EM_CUBE) {
            verts_count = CUBE_VERTS_COUNT;
            verts = CUBE_VERTS;
        }
        else if (group->shape == EM_PYRAMID) {
            verts_count = PYRAMID_VERTS_COUNT;
            verts = PYRAMID_VERTS;
        }
        else {
            verts_count = SPHERE_VERTS_COUNT;
            verts = SPHERE_VERTS;
        }
        graphics_copy_to_buffer(group->vert_buffer, verts, verts_count, 3);
    }

    /* draw agents */
    graphics_draw_triangles_instanced(verts_count, group->max_agents);

    /* draw world box */
    {
        graphics_enable_smooth_line(1);

        shader_program_use(&program_tools);
        shader_program_set_uniform_mat4(&program_tools, 0, &camera.projection);
        shader_program_set_uniform_mat4(&program_tools, 1, &camera.modelview);

        graphics_copy_to_buffer(world_box_pos_buffer, world_box_verts, world_box_verts_count, 3);
        graphics_copy_to_buffer(world_box_col_buffer, world_box_colors, world_box_verts_count, 4);

        graphics_draw_lines(world_box_verts_count);

        graphics_enable_smooth_line(0);
    }
}
