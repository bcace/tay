#include "space.h"
#include "gpu.h"
#include <stdio.h>
#include <assert.h>


static const char *SEE_KERNEL = "\n\
kernel void %s_simple_kernel(global char *seer_agents, int seer_agent_size, global char *seen_agents, int seen_agent_size, int first_seen, float4 radii, global void *see_context) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *seer_agent = (global TayAgentTag *)(seer_agents + i * seer_agent_size);\n\
    float4 seer_p = TAY_AGENT_POSITION(seer_agent);\n\
    global TayAgentTag *seen_agent = (global TayAgentTag *)(seen_agents + first_seen * seen_agent_size);\n\
\n\
    while (seen_agent) {\n\
        float4 seen_p = TAY_AGENT_POSITION(seen_agent);\n\
\n\
        if (seer_agent == seen_agent)\n\
            goto SKIP_SEE;\n\
\n\
        float4 d = seer_p - seen_p;\n\
        if (d.x > radii.x || d.x < -radii.x)\n\
            goto SKIP_SEE;\n\
#if DIMS > 1\n\
        if (d.y > radii.y || d.y < -radii.y)\n\
            goto SKIP_SEE;\n\
#endif\n\
#if DIMS > 2\n\
        if (d.z > radii.z || d.z < -radii.z)\n\
            goto SKIP_SEE;\n\
#endif\n\
#if DIMS > 3\n\
        if (d.w > radii.w || d.w < -radii.w)\n\
            goto SKIP_SEE;\n\
#endif\n\
\n\
        %s(seer_agent, seen_agent, see_context);\n\
\n\
        SKIP_SEE:;\n\
\n\
        seen_agent = seen_agent->next;\n\
    }\n\
}\n";

static const char *ACT_KERNEL = "\n\
kernel void %s_simple_kernel(global char *agents, int agent_size, global void *act_context) {\n\
    %s((global void *)(agents + get_global_id(0) * agent_size), act_context);\n\
}\n";

void gpu_simple_add_source(TayState *state) {
    GpuShared *shared = &state->space.gpu_shared;

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, SEE_KERNEL,
                                           pass->func_name, pass->func_name);
        else if (pass->type == TAY_PASS_ACT)
            shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, ACT_KERNEL,
                                           pass->func_name, pass->func_name);
        else
            assert(0); /* unhandled pass type */
    }
}

void gpu_simple_on_simulation_start(TayState *state) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;
    GpuSimple *simple = &space->gpu_simple;

    /* pass kernels */
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;

        static char pass_kernel_name[512];
        sprintf_s(pass_kernel_name, 512, "%s_simple_kernel", pass->func_name);

        GpuKernel kernel = 0;

        if (pass->type == TAY_PASS_SEE) {
            kernel = gpu_create_kernel(shared->gpu, pass_kernel_name);
            TayGroup *seer_group = state->groups + pass->seer_group;
            TayGroup *seen_group = state->groups + pass->seen_group;
            int first_seen = group_tag_to_index(seen_group, space->first[pass->seen_group]);
            gpu_set_kernel_buffer_argument(kernel, 0, &shared->agent_buffers[pass->seer_group]);
            gpu_set_kernel_value_argument(kernel, 1, &seer_group->agent_size, sizeof(seer_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &shared->agent_buffers[pass->seen_group]);
            gpu_set_kernel_value_argument(kernel, 3, &seen_group->agent_size, sizeof(seen_group->agent_size));
            gpu_set_kernel_value_argument(kernel, 4, &first_seen, sizeof(first_seen));
            gpu_set_kernel_value_argument(kernel, 5, &pass->radii, sizeof(pass->radii));
            gpu_set_kernel_buffer_argument(kernel, 6, &shared->pass_context_buffers[i]);
        }
        else if (pass->type == TAY_PASS_ACT) {
            kernel = gpu_create_kernel(shared->gpu, pass_kernel_name);
            TayGroup *act_group = state->groups + pass->act_group;
            gpu_set_kernel_buffer_argument(kernel, 0, &shared->agent_buffers[pass->act_group]);
            gpu_set_kernel_value_argument(kernel, 1, &act_group->agent_size, sizeof(act_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &shared->pass_context_buffers[i]);
        }
        else
            assert(0); /* unhandled pass type */

        simple->pass_kernels[i] = kernel;
    }
}

void gpu_simple_on_simulation_end(TayState *state) {
    GpuSimple *simple = &state->space.gpu_simple;
    for (int i = 0; i < state->passes_count; ++i)
        gpu_release_kernel(simple->pass_kernels[i]);
}

static void _see(TayState *state, int pass_index) {
    GpuShared *shared = &state->space.gpu_shared;
    GpuSimple *simple = &state->space.gpu_simple;
    TayPass *pass = state->passes + pass_index;
    TayGroup *seer_group = state->groups + pass->seer_group;
    gpu_enqueue_kernel(shared->gpu, simple->pass_kernels[pass_index], seer_group->capacity);
}

static void _act(TayState *state, int pass_index) {
    GpuShared *shared = &state->space.gpu_shared;
    GpuSimple *simple = &state->space.gpu_simple;
    TayPass *pass = state->passes + pass_index;
    TayGroup *act_group = state->groups + pass->act_group;
    gpu_enqueue_kernel(shared->gpu, simple->pass_kernels[pass_index], act_group->capacity);
}

void gpu_simple_step(TayState *state) {
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            _see(state, i);
        else if (pass->type == TAY_PASS_ACT)
            _act(state, i);
        else
            assert(0); /* unhandled pass type */
    }
}
