#include "space.h"
#include "CL/cl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


static void _add_act_kernel_text(TayPass *pass, OclText *text) {
    ocl_text_append(text, "\n\
kernel void %s(global char *a, constant void *c) {\n\
    global void *agent = a + %d * get_global_id(0);\n\
    %s(agent, c);\n\
}\n\
\n",
    ocl_get_kernel_name(pass),
    pass->act_group->agent_size,
    pass->func_name);
}

int ocl_get_pass_kernel(TayState *state, TayPass *pass) {
    pass->pass_kernel = ocl_create_kernel(state, ocl_get_kernel_name(pass));
    if (!pass->pass_kernel)
        return 0;
    return 1;
}

void ocl_text_append(OclText *text, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    text->length += vsnprintf(text->text + text->length, text->max_length - text->length, fmt, ap);
    va_end(ap);
}

char *ocl_text_reserve(OclText *text, unsigned length) {
    char *t = text->text + text->length;
    text->length += length;
    return t;
}

void ocl_text_add_end_of_string(OclText *text) {
    text->text[text->length++] = '\0';
}

void ocl_add_seen_text(OclText *text, TayPass *pass, int dims) {
    if (pass->seen_group->space.type == TAY_CPU_GRID)
        ocl_grid_add_seen_text(text, pass, dims);
    else
        ocl_simple_add_seen_text(text, pass, dims);
}

int ocl_create_buffers(TayState *state) {

    /* create agent and space buffers */
    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group) || !group->ocl_enabled)
            continue;

        OclCommon *ocl_common = &group->space.ocl_common;

        ocl_common->agent_buffer = ocl_create_read_write_buffer(state, group->capacity * group->agent_size);
        if (!ocl_common->agent_buffer)
            return 0;
        ocl_common->agent_sort_buffer = ocl_create_read_write_buffer(state, group->capacity * group->agent_size);
        if (!ocl_common->agent_sort_buffer)
            return 0;
        ocl_common->space_buffer = ocl_create_read_write_buffer(state, group->space.shared_size);
        if (!ocl_common->space_buffer)
            return 0;
        if (!ocl_fill_buffer(state, ocl_common->space_buffer, group->space.shared_size, 0))
            return 0;
        ocl_common->push_agents = 1;
    }

    /* create pass context buffers */
    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass_is_ocl_enabled(pass)) {
            if (pass->context_size) {
                pass->context_buffer = ocl_create_read_only_buffer(state, pass->context_size);
                if (!pass->context_buffer)
                    return 0;
            }
            else
                pass->context_buffer = 0;
        }
    }

    return 1;
}

int ocl_compile_program(TayState *state) {
    TayOcl *ocl = &state->ocl;

    static OclText text;
    if (text.text == 0) { /* alloc once for the entire process and never dealloc */
        text.max_length = 100000;
        text.text = calloc(1, text.max_length);
    }
    text.text[0] = '\0';
    text.length = 0;

    ocl_text_append(&text, "\n\
#define float4_agent_position(__agent_tag__) (*(global float4 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
#define float3_agent_position(__agent_tag__) (*(global float3 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
#define float2_agent_position(__agent_tag__) (*(global float2 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
\n\
#define float4_agent_min(__agent_tag__) (*(global float4 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
#define float3_agent_min(__agent_tag__) (*(global float3 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
#define float2_agent_min(__agent_tag__) (*(global float2 *)((global TayAgentTag *)(__agent_tag__) + 1))\n\
#define float4_agent_max(__agent_tag__) (*(global float4 *)((global char *)(__agent_tag__) + sizeof(TayAgentTag) + sizeof(float4)))\n\
#define float3_agent_max(__agent_tag__) (*(global float3 *)((global char *)(__agent_tag__) + sizeof(TayAgentTag) + sizeof(float4)))\n\
#define float2_agent_max(__agent_tag__) (*(global float2 *)((global char *)(__agent_tag__) + sizeof(TayAgentTag) + sizeof(float4)))\n\
\n\
typedef struct __attribute__((packed)) TayAgentTag {\n\
    unsigned part_i;\n\
    unsigned cell_agent_i;\n\
} TayAgentTag;\n\
\n\
void tay_memcpy(global char *a, global char *b, unsigned size) {\n\
    unsigned count4 = size / 4;\n\
    unsigned count1 = size - (count4 * 4);\n\
    for (unsigned i = 0; i < count4; ++i) {\n\
        *(global uint *)a = *(global uint *)b;\n\
        a += 4;\n\
        b += 4;\n\
    }\n\
    for (unsigned i = 0; i < count1; ++i)\n\
        a[i] = b[i];\n\
}\n\
\n");

    /* add agent model source text */
    for (unsigned source_i = 0; source_i < ocl->sources_count; ++source_i) {
        FILE *file;
        fopen_s(&file, ocl->sources[source_i], "rb");
        fseek(file, 0, SEEK_END);
        unsigned file_length = ftell(file);
        fseek(file, 0, SEEK_SET);
        fread(ocl_text_reserve(&text, file_length), 1, file_length, file);
        fclose(file);
    }

    /* add sort kernel texts */
    {
        ocl_grid_add_kernel_texts(&text);
    }

    /* add space kernel texts */
    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (!pass_is_ocl_enabled(pass))
            continue;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;

            int min_dims = (seer_space->dims < seen_space->dims) ? seer_space->dims : seen_space->dims;

            if (seer_space->type == TAY_CPU_GRID)
                ocl_grid_add_see_kernel_text(&text, pass, min_dims);
            else
                ocl_simple_add_see_kernel_text(&text, pass, min_dims);
        }
        else if (pass->type == TAY_PASS_ACT)
            _add_act_kernel_text(pass, &text);
    }

    ocl_text_add_end_of_string(&text);

    /* create and compile program */
    ocl->device.program = ocl_create_program(state, &text);
    if (!ocl->device.program)
        return 0;

    return 1;
}

void ocl_on_simulation_end(TayState *state) {
    TayOcl *ocl = &state->ocl;
    if (!ocl->device.enabled)
        return;

    ocl_release_program(ocl->device.program);

    /* release agent and space buffers */

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group) || !group->ocl_enabled)
            continue;

        ocl_release_buffer(group->space.ocl_common.agent_buffer);
        ocl_release_buffer(group->space.ocl_common.agent_sort_buffer);
        ocl_release_buffer(group->space.ocl_common.space_buffer);
    }

    /* release pass context buffers */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass_is_ocl_enabled(pass))
            ocl_release_buffer(pass->context_buffer);
    }
}

void ocl_run_see_kernel(TayState *state, TayPass *pass) {
    if (!state->ocl.device.enabled)
        return;
    if (!ocl_set_buffer_kernel_arg(state, 0, pass->pass_kernel, &pass->seer_group->space.ocl_common.agent_buffer))
        return;
    if (!ocl_set_buffer_kernel_arg(state, 1, pass->pass_kernel, &pass->seen_group->space.ocl_common.agent_buffer))
        return;
    if (!ocl_set_buffer_kernel_arg(state, 2, pass->pass_kernel, &pass->context_buffer))
        return;
    if (!ocl_set_value_kernel_arg(state, 3, pass->pass_kernel, &pass->radii, sizeof(pass->radii)))
        return;
    if (!ocl_set_buffer_kernel_arg(state, 4, pass->pass_kernel, &pass->seen_group->space.ocl_common.space_buffer))
        return;
    if (!ocl_run_kernel(state, pass->pass_kernel, pass->seer_group->space.count, 0))
        return;
    ocl_finish(state);
}

void ocl_run_act_kernel(TayState *state, TayPass *pass) {
    if (!state->ocl.device.enabled) // TODO: we shouldn't have to check this, compile step should take care of this
        return;
    if (!ocl_set_buffer_kernel_arg(state, 0, pass->pass_kernel, &pass->act_group->space.ocl_common.agent_buffer))
        return;
    if (!ocl_set_buffer_kernel_arg(state, 1, pass->pass_kernel, &pass->context_buffer))
        return;
    if (!ocl_run_kernel(state, pass->pass_kernel, pass->act_group->space.count, 0))
        return;
    ocl_finish(state);
}

int ocl_has_ocl_enabled_groups(TayState *state) {
    if (!state->ocl.device.enabled)
        return 0;

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_active(group) && group->ocl_enabled)
            return 1;
    }

    return 0;
}

void ocl_push_agents_non_blocking(TayState *state) {
    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group) || !group->ocl_enabled)
            continue;

        if (group->space.ocl_common.push_agents) {
            ocl_write_buffer_non_blocking(state, group->space.ocl_common.agent_buffer, group->storage, group->space.count * group->agent_size);
            group->space.ocl_common.push_agents = 0;
        }
    }
}

void ocl_push_pass_contexts_non_blocking(TayState *state) {
    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass_is_ocl_enabled(pass) && pass->context_size)
            ocl_write_buffer_non_blocking(state, pass->context_buffer, pass->context, pass->context_size);
    }
}

void ocl_fetch_agents(TayState *state) {
    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group) || !group->ocl_enabled)
            continue;

        ocl_read_buffer_non_blocking(state, group->space.ocl_common.agent_buffer, group->storage, group->space.count * group->agent_size);
    }

    ocl_finish(state);
}

void ocl_add_source(TayState *state, const char *path) {
    TayOcl *ocl = &state->ocl;
    if (ocl->sources_count < OCL_MAX_SOURCES) {
        strcpy_s(ocl->sources[ocl->sources_count], OCL_MAX_PATH, path);
        ++ocl->sources_count;
    }
}

char *ocl_get_kernel_name(TayPass *pass) {
    static char kernel_name[TAY_MAX_FUNC_NAME + 32];
    if (pass->type == TAY_PASS_SEE)
        sprintf_s(kernel_name, TAY_MAX_FUNC_NAME + 32, "%s_kernel_%u_%u", pass->func_name, pass->seer_group->id, pass->seen_group->id);
    else
        sprintf_s(kernel_name, TAY_MAX_FUNC_NAME + 32, "%s_kernel_%u", pass->func_name, pass->act_group->id);
    return kernel_name;
}
