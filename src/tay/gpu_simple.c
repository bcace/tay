#include "space.h"
#include "gpu.h"
#include <stdio.h>
#include <assert.h>


// TODO: add dead agent checks for all kernels
static const char *SEE_KERNEL = "\n\
kernel void %s_simple_kernel(global char *seer_agents, int seer_agent_size, global char *seen_agents, int seen_agent_size, float4 radii, global void *see_context) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *seer_agent = (global TayAgentTag *)(seer_agents + i * seer_agent_size);\n\
    float4 seer_p = TAY_AGENT_POSITION(seer_agent);\n\
\n\
    int count = get_global_size(0);\n\
    for (int j = 0; j < count; ++j) {\n\
        global TayAgentTag *seen_agent = (global TayAgentTag *)(seen_agents + j * seer_agent_size);\n\
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
    }\n\
}\n\
\n\
kernel void %s_simple_kernel_indirect(global char *seer_agents, int seer_agent_size, global char *seen_agents, int seen_agent_size, int first_seen, float4 radii, global void *see_context) {\n\
    int i = get_global_id(0);\n\
    global TayAgentTag *seer_agent = (global TayAgentTag *)(seer_agents + i * seer_agent_size);\n\
    float4 seer_p = TAY_AGENT_POSITION(seer_agent);\n\
\n\
    if (seer_agent->next == TAY_GPU_DEAD_ADDR)\n\
        return;\n\
\n\
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
    global TayAgentTag *tag = (global TayAgentTag *)(agents + get_global_id(0) * agent_size);\n\
    if (tag->next != TAY_GPU_DEAD_ADDR)\n\
        %s(tag, act_context);\n\
}\n";

void gpu_simple_add_source(TayState *state) {
    GpuShared *shared = &state->space.gpu_shared;

    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            shared->text_size += sprintf_s(shared->text + shared->text_size, TAY_GPU_MAX_TEXT_SIZE - shared->text_size, SEE_KERNEL,
                                           pass->func_name, pass->func_name, pass->func_name, pass->func_name);
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

        if (pass->type == TAY_PASS_SEE) {
            TayGroup *seer_group = state->groups + pass->seer_group;
            TayGroup *seen_group = state->groups + pass->seen_group;

            {
                sprintf_s(pass_kernel_name, 512, "%s_simple_kernel", pass->func_name);
                GpuKernel kernel = gpu_create_kernel(shared->gpu, pass_kernel_name);

                gpu_set_kernel_buffer_argument(kernel, 0, &shared->agent_buffers[pass->seer_group]);
                gpu_set_kernel_value_argument(kernel, 1, &seer_group->agent_size, sizeof(seer_group->agent_size));
                gpu_set_kernel_buffer_argument(kernel, 2, &shared->agent_buffers[pass->seen_group]);
                gpu_set_kernel_value_argument(kernel, 3, &seen_group->agent_size, sizeof(seen_group->agent_size));
                gpu_set_kernel_value_argument(kernel, 4, &pass->radii, sizeof(pass->radii));
                gpu_set_kernel_buffer_argument(kernel, 5, &shared->pass_context_buffers[i]);

                simple->pass_kernels[i] = kernel;
            }

            {
                sprintf_s(pass_kernel_name, 512, "%s_simple_kernel_indirect", pass->func_name);
                GpuKernel kernel = gpu_create_kernel(shared->gpu, pass_kernel_name);

                gpu_set_kernel_buffer_argument(kernel, 0, &shared->agent_buffers[pass->seer_group]);
                gpu_set_kernel_value_argument(kernel, 1, &seer_group->agent_size, sizeof(seer_group->agent_size));
                gpu_set_kernel_buffer_argument(kernel, 2, &shared->agent_buffers[pass->seen_group]);
                gpu_set_kernel_value_argument(kernel, 3, &seen_group->agent_size, sizeof(seen_group->agent_size));
                gpu_set_kernel_value_argument(kernel, 5, &pass->radii, sizeof(pass->radii));
                gpu_set_kernel_buffer_argument(kernel, 6, &shared->pass_context_buffers[i]);

                simple->pass_kernels_indirect[i] = kernel;
            }
        }
        else if (pass->type == TAY_PASS_ACT) {
            TayGroup *act_group = state->groups + pass->act_group;

            sprintf_s(pass_kernel_name, 512, "%s_simple_kernel", pass->func_name);
            GpuKernel kernel = gpu_create_kernel(shared->gpu, pass_kernel_name);

            gpu_set_kernel_buffer_argument(kernel, 0, &shared->agent_buffers[pass->act_group]);
            gpu_set_kernel_value_argument(kernel, 1, &act_group->agent_size, sizeof(act_group->agent_size));
            gpu_set_kernel_buffer_argument(kernel, 2, &shared->pass_context_buffers[i]);

            simple->pass_kernels[i] = kernel;
        }
        else
            assert(0); /* unhandled pass type */
    }
}

void gpu_simple_on_simulation_end(TayState *state) {
    GpuSimple *simple = &state->space.gpu_simple;
    for (int i = 0; i < state->passes_count; ++i)
        gpu_release_kernel(simple->pass_kernels[i]);
}

static void _see(TayState *state, int pass_index, int direct) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;
    GpuSimple *simple = &space->gpu_simple;
    TayPass *pass = state->passes + pass_index;
    TayGroup *seer_group = state->groups + pass->seer_group;
    if (direct)
        gpu_enqueue_kernel(shared->gpu, simple->pass_kernels[pass_index], seer_group->capacity);
    else {
        TayGroup *seen_group = state->groups + pass->seen_group;
        int first_seen = group_tag_to_index(seen_group, space->first[pass->seen_group]);
        gpu_set_kernel_value_argument(simple->pass_kernels_indirect[pass_index], 4, &first_seen, sizeof(first_seen));
        gpu_enqueue_kernel(shared->gpu, simple->pass_kernels_indirect[pass_index], seer_group->capacity);
    }
}

static void _act(TayState *state, int pass_index) {
    GpuShared *shared = &state->space.gpu_shared;
    GpuSimple *simple = &state->space.gpu_simple;
    TayPass *pass = state->passes + pass_index;
    TayGroup *act_group = state->groups + pass->act_group;
    gpu_enqueue_kernel(shared->gpu, simple->pass_kernels[pass_index], act_group->capacity);
}

void gpu_simple_fix_gpu_pointers(TayState *state) {
    Space *space = &state->space;
    GpuShared *shared = &space->gpu_shared;

    int *next_indices = space_get_temp_arena(space, TAY_MAX_AGENTS * sizeof(int));

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;
        if (group->storage) {

            for (TayAgentTag *tag = space->first[i]; tag; tag = tag->next) {
                int this_i = group_tag_to_index(group, tag);
                int next_i = group_tag_to_index(group, tag->next);
                next_indices[this_i] = next_i;
            }

            space_gpu_finish_fixing_group_gpu_pointers(shared, group, i, next_indices);
        }
    }
}

void gpu_simple_step(TayState *state, int direct) {
    for (int i = 0; i < state->passes_count; ++i) {
        TayPass *pass = state->passes + i;
        if (pass->type == TAY_PASS_SEE)
            _see(state, i, direct);
        else if (pass->type == TAY_PASS_ACT)
            _act(state, i);
        else
            assert(0); /* unhandled pass type */
    }
}
