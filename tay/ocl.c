#include "space.h"
#include "CL/cl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


void ocl_init(TayState *state) {
    TayOcl *ocl = &state->ocl;
    ocl->enabled = 0;
    ocl->sources_count = 0;
}

void ocl_enable(TayState *state) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;

    cl_uint platforms_count = 0;
    cl_platform_id platform_ids[8];
    clGetPlatformIDs(8, platform_ids, &platforms_count);

    for (unsigned plat_i = 0; plat_i < platforms_count; ++plat_i) {
        // printf("platform %d:\n", plat_i);

        // char info_text[1024];

        // clGetPlatformInfo(platform_ids[plat_i], CL_PLATFORM_NAME, 1024, info_text, 0);
        // printf("  name: %s\n", info_text);

        // clGetPlatformInfo(platform_ids[plat_i], CL_PLATFORM_VENDOR, 1024, info_text, 0);
        // printf("  vendor: %s\n", info_text);

        // clGetPlatformInfo(platform_ids[plat_i], CL_PLATFORM_VERSION, 1024, info_text, 0);
        // printf("  version: %s\n", info_text);

        // clGetPlatformInfo(platform_ids[plat_i], CL_PLATFORM_EXTENSIONS, 1024, info_text, 0);
        // printf("  extensions: %s\n", info_text);

        cl_uint devices_count = 0;
        cl_device_id device_ids[8];
        clGetDeviceIDs(platform_ids[plat_i], CL_DEVICE_TYPE_ALL, 8, device_ids, &devices_count);

        for (unsigned dev_i = 0; dev_i < devices_count; ++dev_i) {
            // printf("    device %d:\n", dev_i);

            cl_device_type device_t;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_TYPE, sizeof(cl_device_type), &device_t, 0);
            // if (device_t == CL_DEVICE_TYPE_CPU)
            //     printf("      type: CPU\n");
            // else if (device_t == CL_DEVICE_TYPE_GPU)
            //     printf("      type: GPU\n");
            // else if (device_t == CL_DEVICE_TYPE_ACCELERATOR)
            //     printf("      type: accelerator\n");
            // else
            //     printf("      type: unknown\n");

            // clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_NAME, 1024, info_text, 0);
            // printf("      device name: %s\n", info_text);

            // clGetDeviceInfo(device_ids[dev_i], CL_DRIVER_VERSION, 1024, info_text, 0);
            // printf("      driver version: %s\n", info_text);

            // clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_VERSION, 1024, info_text, 0);
            // printf("      OpenCL version: %s\n", info_text);

            // clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_EXTENSIONS, 1024, info_text, 0);
            // printf("      OpenCL extensions: %s\n", info_text);

            cl_uint max_compute_units;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(max_compute_units), &max_compute_units, 0);
            // printf("      max compute units: %d\n", max_compute_units);

            cl_uint max_work_item_dims;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(max_work_item_dims), &max_work_item_dims, 0);
            // printf("      max work item dimensions: %d\n", max_work_item_dims);

            cl_ulong max_work_item_sizes[4];
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work_item_sizes), max_work_item_sizes, 0);
            // for (unsigned dim_i = 0; dim_i < max_work_item_dims; ++dim_i)
            //     printf("        max work item size: %llu\n", max_work_item_sizes[dim_i]);

            cl_ulong max_workgroup_size;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_workgroup_size), &max_workgroup_size, 0);
            // printf("      max work group size: %llu\n", max_workgroup_size);

            cl_bool device_available;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_AVAILABLE, sizeof(device_available), &device_available, 0);
            // printf("      device available: %s\n", device_available ? "yes" : "no");

            cl_bool compiler_available;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_COMPILER_AVAILABLE, sizeof(compiler_available), &compiler_available, 0);
            // printf("      compiler available: %s\n", compiler_available ? "yes" : "no");

            cl_bool linker_available;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_LINKER_AVAILABLE, sizeof(linker_available), &linker_available, 0);
            // printf("      linker available: %s\n", linker_available ? "yes" : "no");

            cl_ulong global_mem_size;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, 0);
            // printf("      global memory size: %llu\n", global_mem_size);

            cl_ulong local_mem_size;
            clGetDeviceInfo(device_ids[dev_i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem_size), &local_mem_size, 0);
            // printf("      local memory size: %llu\n", local_mem_size);

            if (device_t == CL_DEVICE_TYPE_GPU && device_available && compiler_available && linker_available) {
                ocl->device_id = device_ids[dev_i];
                ocl->enabled = 1;
                ocl->global_mem_size = global_mem_size;
                ocl->local_mem_size = local_mem_size;
                ocl->max_compute_units = max_compute_units;
                ocl->max_work_item_dims = max_work_item_dims;
                for (unsigned dim_i = 0; dim_i < max_work_item_dims; ++dim_i)
                    ocl->max_work_item_sizes[dim_i] = max_work_item_sizes[dim_i];
                ocl->max_workgroup_size = max_workgroup_size;
            }
        }
    }

    cl_int err;

    ocl->context = clCreateContext(NULL, 1, &(cl_device_id)ocl->device_id, NULL, NULL, &err);
    if (err) {
        ocl->enabled = 0;
        tay_set_error(state, TAY_ERROR_OCL);
    }

    ocl->queue = clCreateCommandQueueWithProperties(ocl->context, ocl->device_id, 0, &err);
    if (err) {
        ocl->enabled = 0;
        tay_set_error(state, TAY_ERROR_OCL);
    }

    #endif
}

void ocl_disable(TayState *state) {
    #ifdef TAY_OCL

    if (state->ocl.enabled) {
        clReleaseContext(state->ocl.context);
        state->ocl.enabled = 0;
    }

    #endif
}

static void _add_act_kernel_text(TayPass *pass, OclText *text) {
    #ifdef TAY_OCL

    ocl_text_append(text, "\n\
kernel void %s(global char *a, constant void *c) {\n\
    global void *agent = a + %d * get_global_id(0);\n\
    %s(agent, c);\n\
}\n\
\n",
    ocl_get_kernel_name(pass),
    pass->act_group->agent_size,
    pass->func_name);

    #endif
}

int ocl_get_pass_kernel(TayState *state, TayPass *pass) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    pass->pass_kernel = clCreateKernel(ocl->program, ocl_get_kernel_name(pass), &err);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "pass kernel clCreateKernel error");
        return 0;
    }

    return 1;

    #else
    return 0;
    #endif
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

        if (pass_is_ocl(pass)) {
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
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

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

        if (!pass_is_ocl(pass))
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
    ocl->program = clCreateProgramWithSource(ocl->context, 1, &text.text, 0, &err);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "OCL program compile clCreateProgramWithSource error");
        return 0;
    }

    err = clBuildProgram(ocl->program, 1, &(cl_device_id)ocl->device_id, 0, 0, 0);
    if (err) {
        clGetProgramBuildInfo(ocl->program, ocl->device_id, CL_PROGRAM_BUILD_LOG, text.max_length - text.length, text.text + text.length, 0);
        fprintf(stderr, text.text + text.length);

        tay_set_error2(state, TAY_ERROR_OCL, "OCL program compile clBuildProgram error");
        return 0;
    }

    return 1;

    #else
    return 0;
    #endif
}

void ocl_on_simulation_end(TayState *state) {
    TayOcl *ocl = &state->ocl;
    if (!ocl->enabled)
        return;

    clReleaseProgram(ocl->program);

    /* release agent and space buffers */

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group_is_ocl(group)) {
            clReleaseMemObject(group->space.ocl_common.agent_buffer);
            clReleaseMemObject(group->space.ocl_common.agent_sort_buffer);
            clReleaseMemObject(group->space.ocl_common.space_buffer);
        }
    }

    /* release pass context buffers */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass_is_ocl(pass))
            clReleaseMemObject(pass->context_buffer);
    }
}

void ocl_on_run_start(TayState *state) {
    if (!state->ocl.enabled)
        return;

    /* push agents if they have been modified (push_agents flag) */
    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group) || !group->ocl_enabled)
            continue;

        OclCommon *ocl_common = &group->space.ocl_common;

        if (ocl_common->push_agents) {
            ocl_write_buffer_non_blocking(state, ocl_common->agent_buffer, group->storage, group->space.count * group->agent_size);
            ocl_common->push_agents = 0;
        }
    }

    /* push pass contexts */
    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass_is_ocl(pass) && pass->context_size)
            ocl_write_buffer_non_blocking(state, pass->context_buffer, pass->context, pass->context_size);
    }

    ocl_finish(state);
}

void ocl_on_run_end(TayState *state) {
    ocl_fetch_agents(state);
}

void ocl_sort(TayState *state) {
    if (!state->ocl.enabled)
        return;

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;

        if (group_is_inactive(group) || !group->ocl_enabled)
            continue;

        if (group->space.type == TAY_CPU_GRID)
            ocl_grid_run_sort_kernel(state, group);
    }
}

void ocl_unsort(TayState *state) {
    if (!state->ocl.enabled)
        return;

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayGroup *group = state->groups + i;

        if (group_is_inactive(group) || !group->ocl_enabled)
            continue;

        if (group->space.type == TAY_CPU_GRID)
            ocl_grid_run_unsort_kernel(state, group);
    }
}

void ocl_run_see_kernel(TayState *state, TayPass *pass) {
    if (!state->ocl.enabled)
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
    if (!state->ocl.enabled) // TODO: we shouldn't have to check this, compile step should take care of this
        return;
    if (!ocl_set_buffer_kernel_arg(state, 0, pass->pass_kernel, &pass->act_group->space.ocl_common.agent_buffer))
        return;
    if (!ocl_set_buffer_kernel_arg(state, 1, pass->pass_kernel, &pass->context_buffer))
        return;
    if (!ocl_run_kernel(state, pass->pass_kernel, pass->act_group->space.count, 0))
        return;
    ocl_finish(state);
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

/*
** OpenCL wrappers
**
** If they return 0 that's an error. Mostly they return either a void ptr as buffer or int as a bool.
*/

int ocl_read_buffer_blocking(TayState *state, void *ocl_buffer, void *buffer, unsigned size) {
#ifdef TAY_OCL
    cl_int err = clEnqueueReadBuffer(state->ocl.queue, ocl_buffer, CL_BLOCKING, 0, size, buffer, 0, 0, 0);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clEnqueueReadBuffer");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int ocl_read_buffer_non_blocking(TayState *state, void *ocl_buffer, void *buffer, unsigned size) {
#ifdef TAY_OCL
    cl_int err = clEnqueueReadBuffer(state->ocl.queue, ocl_buffer, CL_NON_BLOCKING, 0, size, buffer, 0, 0, 0);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clEnqueueReadBuffer");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int ocl_write_buffer_blocking(TayState *state, void *ocl_buffer, void *buffer, unsigned size) {
#ifdef TAY_OCL
    cl_int err = clEnqueueWriteBuffer(state->ocl.queue, ocl_buffer, CL_BLOCKING, 0, size, buffer, 0, 0, 0);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clEnqueueWriteBuffer");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int ocl_write_buffer_non_blocking(TayState *state, void *ocl_buffer, void *buffer, unsigned size) {
#ifdef TAY_OCL
    cl_int err = clEnqueueWriteBuffer(state->ocl.queue, ocl_buffer, CL_NON_BLOCKING, 0, size, buffer, 0, 0, 0);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clEnqueueWriteBuffer");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int ocl_fill_buffer(TayState *state, void *buffer, unsigned size, int pattern) {
#ifdef TAY_OCL
    static int pattern_l;
    pattern_l = pattern;
    cl_int err = clEnqueueFillBuffer(state->ocl.queue, buffer, &pattern_l, sizeof(pattern_l), 0, size, 0, 0, 0);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clEnqueueFillBuffer");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int ocl_finish(TayState *state) {
#ifdef TAY_OCL
    cl_int err = clFinish(state->ocl.queue);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clFinish");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int ocl_set_buffer_kernel_arg(TayState *state, unsigned arg_index, void *kernel, void **buffer) {
#ifdef TAY_OCL
    cl_int err = clSetKernelArg(kernel, arg_index, sizeof(void *), buffer);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clSetKernelArg");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int ocl_set_value_kernel_arg(TayState *state, unsigned arg_index, void *kernel, void *value, unsigned value_size) {
#ifdef TAY_OCL
    cl_int err = clSetKernelArg(kernel, arg_index, value_size, value);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clSetKernelArg");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int ocl_run_kernel(TayState *state, void *kernel, unsigned global_size, unsigned local_size) {
#ifdef TAY_OCL
    static unsigned long long global_size_l;
    global_size_l = global_size;
    static unsigned long long local_size_l;
    local_size_l = local_size;
    cl_int err = clEnqueueNDRangeKernel(state->ocl.queue, kernel, 1, 0, &global_size_l, local_size ? &local_size_l : 0, 0, 0, 0);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clEnqueueNDRangeKernel");
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

void *ocl_create_read_write_buffer(TayState *state, unsigned size) {
#ifdef TAY_OCL
    cl_int err;
    void *buffer = clCreateBuffer(state->ocl.context, CL_MEM_READ_WRITE, size, NULL, &err);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clCreateBuffer");
        return 0;
    }
    return buffer;
#else
    return 0;
#endif
}

void *ocl_create_read_only_buffer(TayState *state, unsigned size) {
#ifdef TAY_OCL
    cl_int err;
    void *buffer = clCreateBuffer(state->ocl.context, CL_MEM_READ_ONLY, size, NULL, &err);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clCreateBuffer");
        return 0;
    }
    return buffer;
#else
    return 0;
#endif
}
