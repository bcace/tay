#include "space.h"
#include "CL/cl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OCL_MAX_BOX_THREADS 1024


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

static unsigned _add_act_kernel_text(TayPass *pass, char *text, unsigned remaining_space) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
kernel void %s(global char *a, constant void *c) {\n\
    global void *agent = a + %d * get_global_id(0);\n\
    %s(agent, c);\n\
}\n\
\n",
    ocl_get_kernel_name(pass),
    pass->act_group->agent_size,
    pass->func_name);

    return length;

    #else
    return 0u;
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

void ocl_on_simulation_start(TayState *state) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    /* create agent buffers */

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group_is_ocl(group)) {
            OclCommon *ocl_common = &group->space.ocl_common;

            ocl_common->agent_buffer = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE, group->capacity * group->agent_size, NULL, &err);
            if (err)
                printf("clCreateBuffer error (agent buffer)\n");

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

    /* create box buffer */

    ocl->box_buffer = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE, OCL_MAX_BOX_THREADS * sizeof(Box), NULL, &err);
    if (err)
        printf("clCreateBuffer error (agent buffer)\n");

    /* compose source */

    const unsigned MAX_TEXT_LENGTH = 100000;
    unsigned text_length = 0;
    char *text = calloc(1, MAX_TEXT_LENGTH);

    text_length = sprintf_s(text, MAX_TEXT_LENGTH, "\n\
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
typedef struct __attribute__((packed)) Box {\n\
    float4 min;\n\
    float4 max;\n\
} Box;\n\
\n\
kernel void point_box_kernel_3(global char *agents, unsigned agent_size, unsigned agents_count, global Box *boxes) {\n\
    unsigned thread_i = get_global_id(0);\n\
    unsigned threads_count = get_global_size(0);\n\
\n\
    float4 min = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};\n\
    float4 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};\n\
    for (unsigned a_i = thread_i; a_i < agents_count; a_i += threads_count) {\n\
        float4 a_p = float4_agent_position(agents + agent_size * a_i);\n\
        if (a_p.x < min.x)\n\
            min.x = a_p.x;\n\
        if (a_p.x > max.x)\n\
            max.x = a_p.x;\n\
        if (a_p.y < min.y)\n\
            min.y = a_p.y;\n\
        if (a_p.y > max.y)\n\
            max.y = a_p.y;\n\
        if (a_p.z < min.z)\n\
            min.z = a_p.z;\n\
        if (a_p.z > max.z)\n\
            max.z = a_p.z;\n\
    }\n\
    boxes[thread_i].min = min;\n\
    boxes[thread_i].max = max;\n\
\n\
    barrier(CLK_GLOBAL_MEM_FENCE);\n\
\n\
    if (thread_i == 0) {\n\
        Box box = boxes[0];\n\
        for (unsigned thread_i = 1; thread_i < threads_count; ++thread_i) {\n\
            Box other_box = boxes[thread_i];\n\
            if (other_box.min.x < box.min.x)\n\
                box.min.x = other_box.min.x;\n\
            if (other_box.max.x > box.max.x)\n\
                box.max.x = other_box.max.x;\n\
            if (other_box.min.y < box.min.y)\n\
                box.min.y = other_box.min.y;\n\
            if (other_box.max.y > box.max.y)\n\
                box.max.y = other_box.max.y;\n\
            if (other_box.min.z < box.min.z)\n\
                box.min.z = other_box.min.z;\n\
            if (other_box.max.z > box.max.z)\n\
                box.max.z = other_box.max.z;\n\
        }\n\
        boxes[thread_i] = box;\n\
    }\n\
}\n\
\n");

    /* add agent model source text */

    for (unsigned source_i = 0; source_i < ocl->sources_count; ++source_i) {
        FILE *file;
        fopen_s(&file, ocl->sources[source_i], "rb");
        fseek(file, 0, SEEK_END);
        unsigned file_length = ftell(file);
        fseek(file, 0, SEEK_SET);
        fread(text + text_length, 1, file_length, file);
        fclose(file);
        text_length += file_length;
    }

    /* add space kernel texts */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;

            int min_dims = (seer_space->dims < seen_space->dims) ? seer_space->dims : seen_space->dims;

            if (seer_space->type == seen_space->type) {
                if (seer_space->type == TAY_OCL_SIMPLE)
                    text_length += ocl_simple_add_see_kernel_text(pass, text + text_length, MAX_TEXT_LENGTH - text_length, min_dims);
                else if (seer_space->type == TAY_OCL_GRID)
                    ; //text_length += ocl_grid_add_see_kernel_text(pass, text + text_length, MAX_TEXT_LENGTH - text_length, min_dims);
                else
                    ; // ERROR: not implemented
            }
            else
                ; // ERROR: not implemented
        }
        else if (pass->type == TAY_PASS_ACT) {
            if (group_is_ocl(pass->act_group))
                text_length += _add_act_kernel_text(pass, text + text_length, MAX_TEXT_LENGTH - text_length);
        }
    }

    text[text_length] = '\0';

    /* create and compile program */

    ocl->program = clCreateProgramWithSource(ocl->context, 1, &text, 0, &err);
    if (err) {
        printf("clCreateProgramWithSource error\n");
    }

    err = clBuildProgram(ocl->program, 1, &(cl_device_id)ocl->device_id, 0, 0, 0);
    if (err) {
        static char build_log[10024];
        clGetProgramBuildInfo(ocl->program, ocl->device_id, CL_PROGRAM_BUILD_LOG, 10024, build_log, 0);
        fprintf(stderr, build_log);
    }

    free(text);

    /* get kernels from compiled programs */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;

            if (seer_space->type == seen_space->type) {
                if (group_is_ocl(pass->seer_group))
                    _get_pass_kernel(state, pass);
            }
            else
                ; // ERROR: not implemented
        }
        else if (pass->type == TAY_PASS_ACT) {
            if (group_is_ocl(pass->act_group))
                _get_pass_kernel(state, pass);
        }
        else
            ; // ERROR: not implemented
    }

    /* get box kernels */

    ocl->point_box_kernel_3 = clCreateKernel(ocl->program, "point_box_kernel_3", &err);
    if (err)
        printf("clCreateKernel error (point_box_kernel_3)\n");

    #endif
}

void ocl_on_simulation_end(TayState *state) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;

    clReleaseProgram(ocl->program);

    /* release agent buffers */

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group_is_ocl(group))
            clReleaseMemObject(group->space.ocl_common.agent_buffer);
    }

    /* release box buffer */

    clReleaseMemObject(ocl->box_buffer);

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

const char *ocl_pairing_prologue(int seer_is_point, int seen_is_point) {
    if (seer_is_point && seen_is_point)
        return "\n\
global void *a = a_agents + a_size * a_i;\n\
float4 a_p = float4_agent_position(a);\n\
\n\
for (unsigned b_i = b_beg; b_i < b_end; ++b_i) {\n\
    global void *b = b_agents + b_size * b_i;\n\
    float4 b_p = float4_agent_position(b);\n\
\n";
    else if (seer_is_point)
        return "\n\
global void *a = a_agents + a_size * a_i;\n\
float4 a_p = float4_agent_position(a);\n\
\n\
for (unsigned b_i = b_beg; b_i < b_end; ++b_i) {\n\
    global void *b = b_agents + b_size * b_i;\n\
    float4 b_min = float4_agent_min(b);\n\
    float4 b_max = float4_agent_max(b);\n\
\n";
    else if (seen_is_point)
        return "\n\
global void *a = a_agents + a_size * a_i;\n\
float4 a_min = float4_agent_min(a);\n\
float4 a_max = float4_agent_max(a);\n\
\n\
for (unsigned b_i = b_beg; b_i < b_end; ++b_i) {\n\
    global void *b = b_agents + b_size * b_i;\n\
    float4 b_p = float4_agent_position(b);\n\
\n";
    else
        return "\n\
global void *a = a_agents + a_size * a_i;\n\
float4 a_min = float4_agent_min(a);\n\
float4 a_max = float4_agent_max(a);\n\
\n\
for (unsigned b_i = b_beg; b_i < b_end; ++b_i) {\n\
    global void *b = b_agents + b_size * b_i;\n\
    float4 b_min = float4_agent_min(b);\n\
    float4 b_max = float4_agent_max(b);\n\
\n";
}

const char *ocl_pairing_epilogue() {
    return "\n\
    SKIP_SEE:;\n\
}\n";
}

const char *ocl_self_see_text(int same_group, int self_see) {
    if (same_group && !self_see)
        return "\n\
    if (a_i == b_i)\n\
        goto SKIP_SEE;\n\
\n";
}

const char *_point_point(int dims) {
    if (dims == 1) {
        return "\n\
    float dx = a_p.x - b_p.x;\n\
    if (dx < -radii.x || dx > radii.x)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else if (dims == 2) {
        return "\n\
    float dx = a_p.x - b_p.x;\n\
    if (dx < -radii.x || dx > radii.x)\n\
        goto SKIP_SEE;\n\
    float dy = a_p.y - b_p.y;\n\
    if (dy < -radii.y || dy > radii.y)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else if (dims == 3) {
        return "\n\
    float dx = a_p.x - b_p.x;\n\
    if (dx < -radii.x || dx > radii.x)\n\
        goto SKIP_SEE;\n\
    float dy = a_p.y - b_p.y;\n\
    if (dy < -radii.y || dy > radii.y)\n\
        goto SKIP_SEE;\n\
    float dz = a_p.z - b_p.z;\n\
    if (dz < -radii.z || dz > radii.z)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else {
        return "\n\
    float dx = a_p.x - b_p.x;\n\
    if (dx < -radii.x || dx > radii.x)\n\
        goto SKIP_SEE;\n\
    float dy = a_p.y - b_p.y;\n\
    if (dy < -radii.y || dy > radii.y)\n\
        goto SKIP_SEE;\n\
    float dz = a_p.z - b_p.z;\n\
    if (dz < -radii.z || dz > radii.z)\n\
        goto SKIP_SEE;\n\
    float dw = a_p.w - b_p.w;\n\
    if (dw < -radii.w || dw > radii.w)\n\
        goto SKIP_SEE;\n\
\n";
    }
}

const char *_point_nonpoint(int dims) {
    if (dims == 1) {
        return "\n\
    float d;\n\
    d = a_p.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else if (dims == 2) {
        return "\n\
    float d;\n\
    d = a_p.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_p.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else if (dims == 3) {
        return "\n\
    float d;\n\
    d = a_p.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_p.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_p.z - b_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_min.z - a_p.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else {
        return "\n\
    float d;\n\
    d = a_p.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_p.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_p.z - b_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_min.z - a_p.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = a_p.w - b_max.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
    d = b_min.w - a_p.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
\n";
    }
}

const char *_nonpoint_point(int dims) {
    if (dims == 1) {
        return "\n\
    float d;\n\
    d = b_p.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.x - b_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else if (dims == 2) {
        return "\n\
    float d;\n\
    d = b_p.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.x - b_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_p.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else if (dims == 3) {
        return "\n\
    float d;\n\
    d = b_p.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.x - b_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_p.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_p.z - a_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = a_min.z - b_p.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else {
        return "\n\
    float d;\n\
    d = b_p.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.x - b_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_p.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_p.z - a_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = a_min.z - b_p.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_p.w - a_max.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
    d = a_min.w - b_p.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
\n";
    }
}

const char *_nonpoint_nonpoint(int dims) {
    if (dims == 1) {
        return "\n\
    float d;\n\
    d = a_min.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else if (dims == 2) {
        return "\n\
    float d;\n\
    d = a_min.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
\n";
    }
    if (dims == 1) {
        return "\n\
    float d;\n\
    d = a_min.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.z - b_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_min.z - a_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
\n";
    }
    else {
        return "\n\
    float d;\n\
    d = a_min.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.z - b_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_min.z - a_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = a_min.w - b_max.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
    d = b_min.w - a_max.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
\n";
    }
}

const char *ocl_pairing_text(int seer_is_point, int seen_is_point, int dims) {
    if (seer_is_point == seen_is_point)
        return (seer_is_point) ? _point_point(dims) : _nonpoint_nonpoint(dims);
    else
        return (seer_is_point) ? _point_nonpoint(dims) : _nonpoint_point(dims);
}

char *ocl_get_kernel_name(TayPass *pass) {
    static char kernel_name[TAY_MAX_FUNC_NAME + 32];
    if (pass->type == TAY_PASS_SEE)
        sprintf_s(kernel_name, TAY_MAX_FUNC_NAME + 32, "%s_kernel_%u_%u", pass->func_name, pass->seer_group->id, pass->seen_group->id);
    else
        sprintf_s(kernel_name, TAY_MAX_FUNC_NAME + 32, "%s_kernel_%u", pass->func_name, pass->act_group->id);
    return kernel_name;
}

void ocl_update_space_box(TayState *state, TayGroup *group) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;
    cl_int err;

    int wavefronts_count = 8;
    int threads_count = 64 * wavefronts_count; /* also stride */

    if (threads_count > OCL_MAX_BOX_THREADS)
        threads_count = OCL_MAX_BOX_THREADS;

    err = clSetKernelArg(ocl->point_box_kernel_3, 0, sizeof(void *), &group->space.ocl_common.agent_buffer);
    if (err)
        printf("clSetKernelArg error (point_box_kernel_3 agent buffer)\n");

    err = clSetKernelArg(ocl->point_box_kernel_3, 1, sizeof(group->agent_size), &group->agent_size);
    if (err)
        printf("clSetKernelArg error (point_box_kernel_3 agent size)\n");

    err = clSetKernelArg(ocl->point_box_kernel_3, 2, sizeof(group->space.count), &group->space.count);
    if (err)
        printf("clSetKernelArg error (point_box_kernel_3 agents count)\n");

    err = clSetKernelArg(ocl->point_box_kernel_3, 3, sizeof(void *), &ocl->box_buffer);
    if (err)
        printf("clSetKernelArg error (point_box_kernel_3 box buffer)\n");

    unsigned long long global_work_size = threads_count;

    err = clEnqueueNDRangeKernel(ocl->queue, ocl->point_box_kernel_3, 1, 0, &global_work_size, 0, 0, 0, 0);
    if (err)
        printf("clEnqueueNDRangeKernel error (point_box_kernel_3)\n");

    clFinish(ocl->queue);

    #endif
}
