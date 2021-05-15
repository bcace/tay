#include "space.h"
#include "CL/cl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

void ocl_on_simulation_start(TayState *state) {
    #ifdef TAY_OCL

    TayOcl *ocl = &state->ocl;

    /*
    * create agent buffers
    */

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group->space.type == TAY_OCL_SIMPLE) {
            OclSimple *simple = &group->space.ocl_simple;
            cl_int err;

            simple->agent_buffer = clCreateBuffer(ocl->context, CL_MEM_READ_WRITE, (size_t)(group->capacity * group->agent_size), NULL, &err);
            if (err)
                printf("clCreateBuffer error (agent buffer)\n");

            simple->push_agents = 1;
        }
    }

    /*
    * create pass context buffers
    */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE && pass->seer_group->space.type == TAY_OCL_SIMPLE ||
            pass->type == TAY_PASS_ACT && pass->act_group->space.type == TAY_OCL_SIMPLE) {
            cl_int err;

            pass->context_buffer = clCreateBuffer(ocl->context, CL_MEM_READ_ONLY, pass->context_size, NULL, &err);
            if (err)
                printf("clCreateBuffer error (context buffer)\n");
        }
    }

    /*
    * compose source
    */

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
\n");

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

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;

            if (seer_space->type == seen_space->type) {
                if (seer_space->type == TAY_OCL_SIMPLE) {
                    int min_dims = (seer_space->dims < seen_space->dims) ? seer_space->dims : seen_space->dims;
                    text_length += ocl_simple_add_see_kernel_text(pass, text + text_length, MAX_TEXT_LENGTH - text_length, min_dims);
                }
            }
        }
        else if (pass->type == TAY_PASS_ACT) {
            Space *act_space = &pass->act_group->space;

            if (act_space->type == TAY_OCL_SIMPLE)
                text_length += ocl_simple_add_act_kernel_text(pass, text + text_length, MAX_TEXT_LENGTH - text_length);
        }
    }

    text[text_length] = '\0';

    /*
    * create and compile program
    */

    cl_int err;

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

    /*
    * get kernels from compiled programs
    */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE) {
            Space *seer_space = &pass->seer_group->space;
            Space *seen_space = &pass->seen_group->space;

            if (seer_space->type == seen_space->type) {
                if (seer_space->type == TAY_OCL_SIMPLE)
                    ocl_simple_get_kernel(ocl, pass);
            }
        }
        else if (pass->type == TAY_PASS_ACT) {
            Space *act_space = &pass->act_group->space;

            if (act_space->type == TAY_OCL_SIMPLE)
                ocl_simple_get_kernel(ocl, pass);
        }
    }

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

        if (group->space.type == TAY_OCL_SIMPLE)
            clReleaseMemObject(group->space.ocl_simple.agent_buffer);
    }

    /* release pass context buffers */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE && pass->seer_group->space.type == TAY_OCL_SIMPLE ||
            pass->type == TAY_PASS_ACT && pass->act_group->space.type == TAY_OCL_SIMPLE)
            clReleaseMemObject(pass->context_buffer);
    }

    #endif
}

void ocl_on_run_start(TayState *state) {
    #ifdef TAY_OCL

    /* push agents if they have been modified (push_agents flag) */

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group->space.type == TAY_OCL_SIMPLE) {
            OclSimple *simple = &group->space.ocl_simple;
            cl_int err;

            if (simple->push_agents) {

                err = clEnqueueWriteBuffer(state->ocl.queue, simple->agent_buffer, CL_NON_BLOCKING, 0, group->space.count * group->agent_size, group->storage, 0, 0, 0);
                if (err)
                    printf("clEnqueueWriteBuffer error (agents)\n");

                simple->push_agents = 0;
            }
        }
    }

    /* push pass contexts */

    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (pass->type == TAY_PASS_SEE && pass->seer_group->space.type == TAY_OCL_SIMPLE ||
            pass->type == TAY_PASS_ACT && pass->act_group->space.type == TAY_OCL_SIMPLE) {
            cl_int err;

            err = clEnqueueWriteBuffer(state->ocl.queue, pass->context_buffer, CL_NON_BLOCKING, 0, pass->context_size, pass->context, 0, 0, 0);
            if (err)
                printf("clEnqueueWriteBuffer error (context)\n");
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

void ocl_fetch_agents(TayState *state) {
    #ifdef TAY_OCL

    for (unsigned group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = state->groups + group_i;

        if (group_is_inactive(group))
            continue;

        if (group->space.type == TAY_OCL_SIMPLE) {
            OclSimple *simple = &group->space.ocl_simple;
            cl_int err;

            err = clEnqueueReadBuffer(state->ocl.queue, simple->agent_buffer, CL_NON_BLOCKING, 0, group->space.count * group->agent_size, group->storage, 0, 0, 0);
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
