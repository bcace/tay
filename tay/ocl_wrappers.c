#include "state.h"
#include "CL/cl.h"
#include <stdio.h>

#define TAY_OCL
// #define TAY_OCL_VERBOSE

/*
** OpenCL wrappers
**
** If they return 0 that's an error. Mostly they return either a void ptr as buffer or int as a bool.
*/

void *ocl_create_kernel(TayState *state, char *name) {
#ifdef TAY_OCL
    cl_int err;
    void *kernel = clCreateKernel(state->ocl.device.program, name, &err);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clCreateKernel");
        return 0;
    }
    return kernel;
#else
    return 0;
#endif
}

int ocl_read_buffer_blocking(TayState *state, void *ocl_buffer, void *buffer, unsigned size) {
#ifdef TAY_OCL
    cl_int err = clEnqueueReadBuffer(state->ocl.device.queue, ocl_buffer, CL_BLOCKING, 0, size, buffer, 0, 0, 0);
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
    cl_int err = clEnqueueReadBuffer(state->ocl.device.queue, ocl_buffer, CL_NON_BLOCKING, 0, size, buffer, 0, 0, 0);
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
    cl_int err = clEnqueueWriteBuffer(state->ocl.device.queue, ocl_buffer, CL_BLOCKING, 0, size, buffer, 0, 0, 0);
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
    cl_int err = clEnqueueWriteBuffer(state->ocl.device.queue, ocl_buffer, CL_NON_BLOCKING, 0, size, buffer, 0, 0, 0);
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
    cl_int err = clEnqueueFillBuffer(state->ocl.device.queue, buffer, &pattern_l, sizeof(pattern_l), 0, size, 0, 0, 0);
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
    cl_int err = clFinish(state->ocl.device.queue);
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
    cl_int err = clEnqueueNDRangeKernel(state->ocl.device.queue, kernel, 1, 0, &global_size_l, local_size ? &local_size_l : 0, 0, 0, 0);
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
    void *buffer = clCreateBuffer(state->ocl.device.context, CL_MEM_READ_WRITE, size, NULL, &err);
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
    void *buffer = clCreateBuffer(state->ocl.device.context, CL_MEM_READ_ONLY, size, NULL, &err);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clCreateBuffer");
        return 0;
    }
    return buffer;
#else
    return 0;
#endif
}

void ocl_release_program(void *program) {
#ifdef TAY_OCL
    clReleaseProgram(program);
#endif
}

void ocl_release_buffer(void *buffer) {
#ifdef TAY_OCL
    clReleaseMemObject(buffer);
#endif
}

void *ocl_create_program(TayState *state, OclText *text) {
#ifdef TAY_OCL
    cl_int err;
    void *program = clCreateProgramWithSource(state->ocl.device.context, 1, &text->text, 0, &err);
    if (err) {
        tay_set_error2(state, TAY_ERROR_OCL, "clCreateProgramWithSource");
        return 0;
    }
    err = clBuildProgram(program, 1, &(cl_device_id)state->ocl.device.device, 0, 0, 0);
    if (err) {
        clGetProgramBuildInfo(program, state->ocl.device.device, CL_PROGRAM_BUILD_LOG,
                              text->max_length - text->length, text->text + text->length, 0);
        fprintf(stderr, text->text + text->length);
        tay_set_error2(state, TAY_ERROR_OCL, "clGetProgramBuildInfo");
        return 0;
    }
    return program;
#else
    return 0;
#endif
}

int ocl_init_context(TayState *state, OclDevice *device) {
    device->enabled = 0;

#ifdef TAY_OCL
    cl_uint plat_count = 0;
    cl_platform_id plat_ids[8];
    clGetPlatformIDs(8, plat_ids, &plat_count);

    for (unsigned plat_i = 0; plat_i < plat_count; ++plat_i) {
#ifdef TAY_OCL_VERBOSE
        char info_buf[1024];

        printf("platform %d:\n", plat_i);
        clGetPlatformInfo(plat_ids[plat_i], CL_PLATFORM_NAME, 1024, info_buf, 0);
        printf("  name: %s\n", info_buf);
        clGetPlatformInfo(plat_ids[plat_i], CL_PLATFORM_VENDOR, 1024, info_buf, 0);
        printf("  vendor: %s\n", info_buf);
        clGetPlatformInfo(plat_ids[plat_i], CL_PLATFORM_VERSION, 1024, info_buf, 0);
        printf("  version: %s\n", info_buf);
        clGetPlatformInfo(plat_ids[plat_i], CL_PLATFORM_EXTENSIONS, 1024, info_buf, 0);
        printf("  extensions: %s\n", info_buf);
#endif
        cl_uint dev_count = 0;
        cl_device_id dev_ids[8];
        clGetDeviceIDs(plat_ids[plat_i], CL_DEVICE_TYPE_ALL, 8, dev_ids, &dev_count);

        for (unsigned dev_i = 0; dev_i < dev_count; ++dev_i) {
#ifdef TAY_OCL_VERBOSE
            printf("    device %d:\n", dev_i);
#endif

            cl_device_type dev_type;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_TYPE, sizeof(cl_device_type), &dev_type, 0);
#ifdef TAY_OCL_VERBOSE
            if (dev_type == CL_DEVICE_TYPE_CPU)
                printf("      type: CPU\n");
            else if (dev_type == CL_DEVICE_TYPE_GPU)
                printf("      type: GPU\n");
            else if (dev_type == CL_DEVICE_TYPE_ACCELERATOR)
                printf("      type: accelerator\n");
            else
                printf("      type: unknown\n");
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_NAME, 1024, info_buf, 0);
            printf("      device name: %s\n", info_buf);
            clGetDeviceInfo(dev_ids[dev_i], CL_DRIVER_VERSION, 1024, info_buf, 0);
            printf("      driver version: %s\n", info_buf);
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_VERSION, 1024, info_buf, 0);
            printf("      OpenCL version: %s\n", info_buf);
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_EXTENSIONS, 1024, info_buf, 0);
            printf("      OpenCL extensions: %s\n", info_buf);
#endif
            cl_uint max_compute_units;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(max_compute_units), &max_compute_units, 0);
#ifdef TAY_OCL_VERBOSE
            printf("      max compute units: %d\n", max_compute_units);
#endif
            cl_uint max_work_item_dims;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(max_work_item_dims), &max_work_item_dims, 0);
#ifdef TAY_OCL_VERBOSE
            printf("      max work item dimensions: %d\n", max_work_item_dims);
#endif
            cl_ulong max_work_item_sizes[4];
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(max_work_item_sizes), max_work_item_sizes, 0);
#ifdef TAY_OCL_VERBOSE
            for (unsigned dim_i = 0; dim_i < max_work_item_dims; ++dim_i)
                printf("        max work item size: %llu\n", max_work_item_sizes[dim_i]);
#endif
            cl_ulong max_workgroup_size;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_workgroup_size), &max_workgroup_size, 0);
#ifdef TAY_OCL_VERBOSE
            printf("      max work group size: %llu\n", max_workgroup_size);
#endif
            cl_bool device_available;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_AVAILABLE, sizeof(device_available), &device_available, 0);
#ifdef TAY_OCL_VERBOSE
            printf("      device available: %s\n", device_available ? "yes" : "no");
#endif
            cl_bool compiler_available;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_COMPILER_AVAILABLE, sizeof(compiler_available), &compiler_available, 0);
#ifdef TAY_OCL_VERBOSE
            printf("      compiler available: %s\n", compiler_available ? "yes" : "no");
#endif
            cl_bool linker_available;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_LINKER_AVAILABLE, sizeof(linker_available), &linker_available, 0);
#ifdef TAY_OCL_VERBOSE
            printf("      linker available: %s\n", linker_available ? "yes" : "no");
#endif
            cl_ulong global_mem_size;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, 0);
#ifdef TAY_OCL_VERBOSE
            printf("      global memory size: %llu\n", global_mem_size);
#endif
            cl_ulong local_mem_size;
            clGetDeviceInfo(dev_ids[dev_i], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem_size), &local_mem_size, 0);
#ifdef TAY_OCL_VERBOSE
            printf("      local memory size: %llu\n", local_mem_size);
#endif
            if (dev_type == CL_DEVICE_TYPE_GPU && device_available && compiler_available && linker_available) {
                device->device = dev_ids[dev_i];
                device->enabled = 1;
                device->global_mem_size = global_mem_size;
                device->local_mem_size = local_mem_size;
                device->max_compute_units = max_compute_units;
                device->max_work_item_dims = max_work_item_dims;
                for (unsigned dim_i = 0; dim_i < max_work_item_dims; ++dim_i)
                    device->max_work_item_sizes[dim_i] = max_work_item_sizes[dim_i];
                device->max_workgroup_size = max_workgroup_size;
            }
        }
    }

    if (!device->enabled)
        tay_set_error2(state, TAY_ERROR_OCL, "device not found");
    else {
        cl_int err;
        device->context = clCreateContext(NULL, 1, &(cl_device_id)device->device, NULL, NULL, &err);
        if (err) {
            device->enabled = 0;
            tay_set_error2(state, TAY_ERROR_OCL, "clCreateContext");
        }
        else {
            device->queue = clCreateCommandQueueWithProperties(device->context, device->device, 0, &err);
            if (err) {
                device->enabled = 0;
                tay_set_error2(state, TAY_ERROR_OCL, "clCreateCommandQueueWithProperties");
            }
        }
    }

#endif
    return device->enabled;
}

void ocl_release_context(OclDevice *device) {
    if (device->enabled) {
        clReleaseContext(device->context);
        device->enabled = 0;
    }
}
