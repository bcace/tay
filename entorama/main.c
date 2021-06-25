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
    model_info.init = 0;
    model_load(&model_info, "m_flocking.dll");

    TayState *tay = tay_create_state();

    EntoramaSimulationInfo sim_info;
    sim_info.groups_count = 0;
    model_info.init(&sim_info, tay);

    tay_threads_start(100000); // TODO: remove this!!!
    tay_simulation_start(tay);

    const int max_agents_count = 1000000; // TODO: adapt this to the actual inited model!

    vec3 *inst_vec3_buffers[] = {
        malloc(sizeof(vec3) * max_agents_count),
        malloc(sizeof(vec3) * max_agents_count),
    };
    float *inst_float_buffers[] = {
        malloc(sizeof(float) * max_agents_count),
        malloc(sizeof(float) * max_agents_count),
    };

    while (!quit) {
        graphics_viewport(0, 0, window_w, window_h);
        vec4 bg = color_bg();
        graphics_clear(bg.x, bg.y, bg.z);
        graphics_clear_depth();
        graphics_enable_depth_test(1);

        tay_run(tay, 1);

        /* drawing */
        {
            mat4 perspective;
            graphics_perspective(&perspective, 1.2f, (float)window_w / (float)window_h, 1.0f, 4000.0f);

            vec3 pos, fwd, up;
            pos.x = -2000.0f;
            pos.y = 0.0f;
            pos.z = 0.0f;
            fwd.x = 1.0f;
            fwd.y = 0.0f;
            fwd.z = 0.0f;
            up.x = 0.0f;
            up.y = 0.0f;
            up.z = 1.0f;

            mat4 lookat;
            graphics_lookat(&lookat, pos, fwd, up);

            mat4 projection;
            mat4_multiply(&projection, &perspective, &lookat);

            for (unsigned group_i = 0; group_i < sim_info.groups_count; ++group_i) {
                EntoramaGroupInfo *group_info = sim_info.groups + group_i;

                /* different agent types might use different shaders, if not move this out */
                shader_program_use(&program);
                shader_program_set_uniform_mat4(&program, 0, &projection);

                vec3 *inst_pos = inst_vec3_buffers[0];
                vec3 *inst_dir = inst_vec3_buffers[1];
                float *inst_shd = inst_float_buffers[0];

                for (unsigned agent_i = 0; agent_i < group_info->max_agents; ++agent_i) {
                    char *agent = tay_get_agent(tay, group_info->group, agent_i);
                    inst_pos[agent_i] = *(vec3 *)(agent + sizeof(TayAgentTag));
                    inst_dir[agent_i] = *(vec3 *)(agent + sizeof(TayAgentTag) + sizeof(float4));
                    inst_shd[agent_i] = 0.0f;
                }

                shader_program_set_data_float(&program, 0, PYRAMID_VERTS_COUNT, 3, PYRAMID_VERTS);
                shader_program_set_data_float(&program, 1, group_info->max_agents, 3, inst_pos);
                shader_program_set_data_float(&program, 2, group_info->max_agents, 3, inst_dir);
                shader_program_set_data_float(&program, 3, group_info->max_agents, 1, inst_shd);

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
