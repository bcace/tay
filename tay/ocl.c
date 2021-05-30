#include "space.h"
#include "CL/cl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


void ocl_init(TayState *state) {
    TayOcl *ocl = &state->ocl;
    ocl->active = CL_FALSE;
    ocl->sources_count = 0;

    #ifdef TAY_OCL

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
                ocl->active = CL_TRUE;
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
        ocl->active = 0;
        printf("clCreateContext error\n");
    }

    ocl->queue = clCreateCommandQueueWithProperties(ocl->context, ocl->device_id, 0, &err);
    if (err) {
        ocl->active = 0;
        printf("clCreateCommandQueueWithProperties error\n");
    }

    #endif
}

void ocl_destroy(TayState *state) {
    #ifdef TAY_OCL

    clReleaseContext(state->ocl.context);

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

static void _get_pass_kernel(TayState *state, TayPass *pass) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    pass->pass_kernel = clCreateKernel(ocl->program, ocl_get_kernel_name(pass), &err);
    if (err)
        printf("clCreateKernel error\n");

    #endif
}

void ocl_text_init(OclText *text, unsigned max_length) {
    text->max_length = max_length;
    text->length = 0;
    text->text = calloc(1, text->max_length);
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
    text->text[text->length] = '\0';
}

void ocl_text_destroy(OclText *text) {
    free(text->text);
    text->text = 0;
}

void ocl_add_seen_text(OclText *text, TayPass *pass, int dims) {
    if (pass->seen_group->space.type == TAY_OCL_SIMPLE)
        ocl_simple_add_seen_text(text, pass, dims);
    else if (pass->seen_group->space.type == TAY_OCL_GRID)
        ocl_grid_add_seen_text(text, pass, dims);
    else
        ; // ERROR: not implemented
}

void ocl_on_simulation_start(TayState *state) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    /* create agent and space buffers */

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group_is_ocl(group)) {
            OclCommon *ocl_common = &group->space.ocl_common;

            ocl_common->agent_buffer = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE, group->capacity * group->agent_size, NULL, &err);
            if (err)
                printf("clCreateBuffer error (agent buffer)\n");

            ocl_common->agent_sort_buffer = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE, group->capacity * group->agent_size, NULL, &err);
            if (err)
                printf("clCreateBuffer error (agent sort buffer)\n");

            ocl_common->space_buffer = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE, group->space.shared_size, NULL, &err);
            if (err)
                printf("clCreateBuffer error (space buffer)\n");

            cl_int pattern = 0;
            err = clEnqueueFillBuffer(ocl->queue, ocl_common->space_buffer, &pattern, sizeof(pattern), 0, group->space.shared_size, 0, 0, 0);
            if (err)
                printf("clEnqueueFillBuffer error (space buffer)\n");

            ocl_common->push_agents = 1;
        }
    }

    /* create pass context buffers */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass_is_ocl(pass)) {
            if (pass->context_size) {
                pass->context_buffer = clCreateBuffer(ocl->context, CL_MEM_READ_ONLY, pass->context_size, NULL, &err);
                if (err)
                    printf("clCreateBuffer error (context buffer)\n");
            }
            else
                pass->context_buffer = 0;
        }
    }

    /* compose source */

    OclText text;
    ocl_text_init(&text, 100000);

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

    ocl_grid_add_kernel_texts(&text);

    /* add space kernel texts */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;

            int min_dims = (seer_space->dims < seen_space->dims) ? seer_space->dims : seen_space->dims;

            if (seer_space->type == TAY_OCL_SIMPLE)
                ocl_simple_add_see_kernel_text(&text, pass, min_dims);
            else if (seer_space->type == TAY_OCL_GRID)
                ocl_grid_add_see_kernel_text(&text, pass, min_dims);
            else
                ; // ERROR: not implemented
        }
        else if (pass->type == TAY_PASS_ACT) {
            if (group_is_ocl(pass->act_group))
                _add_act_kernel_text(pass, &text);
        }
    }

    ocl_text_add_end_of_string(&text);

    /* create and compile program */

    ocl->program = clCreateProgramWithSource(ocl->context, 1, &text.text, 0, &err);
    if (err)
        printf("clCreateProgramWithSource error\n");

    err = clBuildProgram(ocl->program, 1, &(cl_device_id)ocl->device_id, 0, 0, 0);
    if (err) {
        static char build_log[10024];
        clGetProgramBuildInfo(ocl->program, ocl->device_id, CL_PROGRAM_BUILD_LOG, 10024, build_log, 0);
        fprintf(stderr, build_log);
    }

    ocl_text_destroy(&text);

    /* get pass kernels from compiled program */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;
            if (group_is_ocl(pass->seer_group))
                _get_pass_kernel(state, pass);
        }
        else if (pass->type == TAY_PASS_ACT) {
            if (group_is_ocl(pass->act_group))
                _get_pass_kernel(state, pass);
        }
        else
            ; // ERROR: not implemented
    }

    /* get other kernels */

    ocl_grid_get_kernels(state);

    #endif
}

void ocl_on_simulation_end(TayState *state) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;

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

    #endif
}

void ocl_on_run_start(TayState *state) {
    #ifdef TAY_OCL

    cl_int err;

    /* push agents if they have been modified (push_agents flag) */

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group_is_ocl(group)) {
            OclCommon *ocl_common = &group->space.ocl_common;

            if (ocl_common->push_agents) {

                err = clEnqueueWriteBuffer(state->ocl.queue, ocl_common->agent_buffer, CL_NON_BLOCKING, 0, group->space.count * group->agent_size, group->storage, 0, 0, 0);
                if (err)
                    printf("clEnqueueWriteBuffer error (agents)\n");

                ocl_common->push_agents = 0;
            }
        }
    }

    /* push pass contexts */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass_is_ocl(pass)) {
            if (pass->context_size) {
                err = clEnqueueWriteBuffer(state->ocl.queue, pass->context_buffer, CL_NON_BLOCKING, 0, pass->context_size, pass->context, 0, 0, 0);
                if (err)
                    printf("clEnqueueWriteBuffer error (context)\n");
            }
        }
    }

    clFinish(state->ocl.queue);

    #endif
}

void ocl_on_run_end(TayState *state) {
    #ifdef TAY_OCL

    ocl_fetch_agents(state);

    #endif
}

void ocl_run_see_kernel(TayState *state, TayPass *pass) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    err = clSetKernelArg(pass->pass_kernel, 0, sizeof(void *), &pass->seer_group->space.ocl_common.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 1, sizeof(void *), &pass->seen_group->space.ocl_common.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 2, sizeof(void *), &pass->context_buffer);
    if (err)
        printf("clSetKernelArg error (context buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 3, sizeof(pass->radii), &pass->radii);
    if (err)
        printf("clSetKernelArg error (radii)\n");

    err = clSetKernelArg(pass->pass_kernel, 4, sizeof(void *), &pass->seen_group->space.ocl_common.space_buffer);
    if (err)
        printf("clSetKernelArg error (space buffer)\n");

    unsigned long long global_work_size = pass->seer_group->space.count;

    err = clEnqueueNDRangeKernel(ocl->queue, pass->pass_kernel, 1, 0, &global_work_size, 0, 0, 0, 0);
    if (err)
        printf("clEnqueueNDRangeKernel error\n");

    err = clFinish(ocl->queue);
    if (err)
        printf("clFinish error\n");

    #endif
}

void ocl_run_act_kernel(TayState *state, TayPass *pass) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    err = clSetKernelArg(pass->pass_kernel, 0, sizeof(void *), &pass->act_group->space.ocl_common.agent_buffer);
    if (err)
        printf("clSetKernelArg error (agent buffer)\n");

    err = clSetKernelArg(pass->pass_kernel, 1, sizeof(void *), &pass->context_buffer);
    if (err)
        printf("clSetKernelArg error (context buffer)\n");

    unsigned long long global_work_size = pass->act_group->space.count;

    err = clEnqueueNDRangeKernel(ocl->queue, pass->pass_kernel, 1, 0, &global_work_size, 0, 0, 0, 0);
    if (err)
        printf("clEnqueueNDRangeKernel error\n");

    err = clFinish(ocl->queue);
    if (err)
        printf("clFinish error\n");

    #endif
}

void ocl_fetch_agents(TayState *state) {
    #ifdef TAY_OCL

    cl_int err;

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group_is_ocl(group)) {
            OclCommon *ocl_common = &group->space.ocl_common;

            err = clEnqueueReadBuffer(state->ocl.queue, ocl_common->agent_buffer, CL_NON_BLOCKING, 0, group->space.count * group->agent_size, group->storage, 0, 0, 0);
            if (err)
                printf("clEnqueueReadBuffer error\n");
        }
    }

    clFinish(state->ocl.queue);

    #endif
}

void ocl_add_source(TayState *state, const char *path) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;

    if (ocl->sources_count < OCL_MAX_SOURCES) {
        strcpy_s(ocl->sources[ocl->sources_count], OCL_MAX_PATH, path);
        ++ocl->sources_count;
    }

    #endif
}

char *ocl_get_kernel_name(TayPass *pass) {
    static char kernel_name[TAY_MAX_FUNC_NAME + 32];
    if (pass->type == TAY_PASS_SEE)
        sprintf_s(kernel_name, TAY_MAX_FUNC_NAME + 32, "%s_kernel_%u_%u", pass->func_name, pass->seer_group->id, pass->seen_group->id);
    else
        sprintf_s(kernel_name, TAY_MAX_FUNC_NAME + 32, "%s_kernel_%u", pass->func_name, pass->act_group->id);
    return kernel_name;
}
