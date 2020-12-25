#include "gpu.h"
#define CL_TARGET_OPENCL_VERSION 200
#include <CL/opencl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


typedef struct GpuContext {
    cl_device_id device;
    cl_context context;
    cl_command_queue commands;
    cl_program program;
} GpuContext;

static int _check_errors(int err, const char *text);

GpuContext *gpu_create(void) {
    GpuContext *c = malloc(sizeof(GpuContext));

    cl_platform_id platform_ids[32];
    cl_uint platforms_count;
    clGetPlatformIDs(1, platform_ids, &platforms_count);

    c->device = 0;
    c->context = 0;
    c->commands = 0;
    c->program = 0;

    cl_int err = clGetDeviceIDs(platform_ids[0], CL_DEVICE_TYPE_GPU, 1, &c->device, 0);
    if (_check_errors(err, "clGetDeviceIDs"))
        return 0;

    c->context = clCreateContext(0, 1, &c->device, 0, 0, &err);
    if (_check_errors(err, "clCreateContext"))
        return 0;

    c->commands = clCreateCommandQueueWithProperties(c->context, c->device, 0, &err);
    if (_check_errors(err, "clCreateCommandQueueWithProperties"))
        return 0;

    return c;
}

int gpu_destroy(GpuContext *c) {
    cl_int err;

    if (c->program) {
        err = clReleaseProgram(c->program);
        if (_check_errors(err, "clReleaseProgram"))
            return err;
    }

    err = clReleaseCommandQueue(c->commands);
    if (_check_errors(err, "clReleaseCommandQueue"))
        return err;

    err = clReleaseContext(c->context);
    if (_check_errors(err, "clReleaseContext"))
        return err;

    free(c);

    return 0;
}

int gpu_build_program(GpuContext *c, const char *source) {
    assert(c->program == 0); // TODO: release old and create a new program here

    cl_int err;
    c->program = clCreateProgramWithSource(c->context, 1, &source, 0, &err);
    if (_check_errors(err, "Failed to create program with source"))
        return err;

    err = clBuildProgram(c->program, 1, &c->device, 0, 0, 0);
    if (_check_errors(err, "Failed to build program")) {

        static char build_log[10024];
        clGetProgramBuildInfo(c->program, c->device, CL_PROGRAM_BUILD_LOG, 10024, build_log, 0);
        fprintf(stderr, build_log);

        return err;
    }

    return 0;
}

GpuKernel gpu_create_kernel(GpuContext *c, const char *name) {
    cl_int err;
    GpuKernel kernel = clCreateKernel(c->program, name, &err);
    if (_check_errors(err, "clCreateKernel"))
        return 0;
    return kernel;
}

static int _set_kernel_argument(GpuKernel kernel, int index, const void *arg, int arg_size) {
    cl_int err = clSetKernelArg(kernel, index, arg_size, arg);
    if (_check_errors(err, "clSetKernelArg"))
        return err;
    return 0;
}

int gpu_set_kernel_value_argument(GpuKernel kernel, int index, void *value, int value_size) {
    return _set_kernel_argument(kernel, index, value, value_size);
}

int gpu_set_kernel_buffer_argument(GpuKernel kernel, int index, GpuBuffer *buffer) {
    return _set_kernel_argument(kernel, index, buffer, sizeof(cl_mem));
}

int gpu_release_kernel(GpuKernel kernel) {
    cl_int err = clReleaseKernel(kernel);
    if (_check_errors(err, "clReleaseKernel"))
        return 1;
    return 0;
}

GpuBuffer gpu_create_buffer(GpuContext *c, GpuMemFlags device, GpuMemFlags host, int size) {
    cl_mem_flags flags = 0;
    if (device == GPU_MEM_READ_ONLY)
        flags |= CL_MEM_READ_ONLY;
    else if (device == GPU_MEM_WRITE_ONLY)
        flags |= CL_MEM_WRITE_ONLY;
    else if (device == GPU_MEM_READ_AND_WRITE)
        flags |= CL_MEM_READ_WRITE;
    else
        assert(0);
    if (host == GPU_MEM_READ_ONLY)
        flags |= CL_MEM_HOST_READ_ONLY;
    else if (host == GPU_MEM_WRITE_ONLY)
        flags |= CL_MEM_HOST_WRITE_ONLY;
    else if (host == GPU_MEM_NONE)
        ;
    else
        assert(0);
    cl_int err;
    cl_mem buffer = clCreateBuffer(c->context, flags, size, 0, &err);
    if (_check_errors(err, "clCreateBuffer"))
        return 0;

    return buffer;
}

int gpu_release_buffer(GpuBuffer buffer) {
    cl_int err = clReleaseMemObject(buffer);
    if (_check_errors(err, "clReleaseMemObject"))
        return 0;
    return err;
}

/* NOTE: blocking refers to the operation itself, not the previous enqueued operations */
int gpu_enqueue_write_buffer(GpuContext *c, GpuBuffer buffer, GpuBlocking blocking, int size, void *arg) {
    cl_int err = clEnqueueWriteBuffer(c->commands, buffer, blocking, 0, size, arg, 0, 0, 0);
    if (_check_errors(err, "clEnqueueWriteBuffer"))
        return 1;
    return 0;
}

int gpu_enqueue_read_buffer(GpuContext *c, GpuBuffer buffer, GpuBlocking blocking, int size, void *arg) {
    cl_int err = clEnqueueReadBuffer(c->commands, buffer, blocking, 0, size, arg, 0, 0, 0);
    if (_check_errors(err, "clEnqueueReadBuffer"))
        return 1;
    return 0;
}

int gpu_enqueue_kernel(GpuContext *c, GpuKernel kernel, int count) {
    size_t wide_count = count;
    cl_int err = clEnqueueNDRangeKernel(c->commands, kernel, 1, 0, &wide_count, 0, 0, 0, 0);
    if (_check_errors(err, "clEnqueueNDRangeKernel"))
        return 1;
    clFinish(c->commands); /* must be here because each kernel represents a iteration phase, and also having a pointer to wide_count local variable wouldn't work */
    return 0;
}

int gpu_enqueue_kernel_nb(GpuContext *c, GpuKernel kernel, long long int *count) {
    cl_int err = clEnqueueNDRangeKernel(c->commands, kernel, 1, 0, (size_t *)count, 0, 0, 0, 0);
    if (_check_errors(err, "clEnqueueNDRangeKernel"))
        return 1;
    return 0;
}

int gpu_finish(GpuContext *c) {
    cl_int err = clFinish(c->commands);
    if (_check_errors(err, "clFinish"))
        return 1;
    return 0;
}

static int _check_errors(int err, const char *func_name) {
    if (err != CL_SUCCESS) {
        if (err == -1) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_DEVICE_NOT_FOUND",
                "if no OpenCL devices that matched device_type were found."
            );
        }
        else if (err == -2) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_DEVICE_NOT_AVAILABLE",
                "if a device in devices is currently not available even though the device was returned by clGetDeviceIDs."
            );
        }
        else if (err == -3) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_COMPILER_NOT _AVAILABLE",
                "if program is created with clCreateProgramWithSource and a compiler is not available i.e. CL_DEVICE_COMPILER_AVAILABLE specified in the table of OpenCL Device Queries for clGetDeviceInfo is set to CL_FALSE."
            );
        }
        else if (err == -4) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_MEM_OBJECT _ALLOCATION_FAILURE",
                "if there is a failure to allocate memory for buffer object."
            );
        }
        else if (err == -5) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_OUT_OF_RESOURCES",
                "if there is a failure to allocate resources required by the OpenCL implementation on the device."
            );
        }
        else if (err == -6) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_OUT_OF_HOST_MEMORY",
                "if there is a failure to allocate resources required by the OpenCL implementation on the host."
            );
        }
        else if (err == -7) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_PROFILING_INFO_NOT _AVAILABLE",
                "if the CL_QUEUE_PROFILING_ENABLE flag is not set for the command-queue, if the execution err of the command identified by event is not CL_COMPLETE or if event is a user event object."
            );
        }
        else if (err == -8) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_MEM_COPY_OVERLAP",
                "if src_buffer and dst_buffer are the same buffer or subbuffer object and the source and destination regions overlap or if src_buffer and dst_buffer are different sub-buffers of the same associated buffer object and they overlap. The regions overlap if src_offset ≤ to dst_offset ≤ to src_offset + size – 1, or if dst_offset ≤ to src_offset ≤ to dst_offset + size – 1."
            );
        }
        else if (err == -9) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_IMAGE_FORMAT_MISMATCH",
                "if src_image and dst_image do not use the same image format."
            );
        }
        else if (err == -10) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_IMAGE_FORMAT_NOT_SUPPORTED",
                "if the image_format is not supported."
            );
        }
        else if (err == -11) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_BUILD_PROGRAM_FAILURE",
                "if there is a failure to build the program executable. This error will be returned if clBuildProgram does not return until the build has completed."
            );
        }
        else if (err == -12) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_MAP_FAILURE",
                "if there is a failure to map the requested region into the host address space. This error cannot occur for image objects created with CL_MEM_USE_HOST_PTR or CL_MEM_ALLOC_HOST_PTR."
            );
        }
        else if (err == -13) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_MISALIGNED_SUB_BUFFER_OFFSET",
                "if a sub-buffer object is specified as the value for an argument that is a buffer object and the offset specified when the sub-buffer object is created is not aligned to CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with queue."
            );
        }
        else if (err == -14) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST",
                "if the execution status of any of the events in event_list is a negative integer value."
            );
        }
        else if (err == -15) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_COMPILE_PROGRAM_FAILURE",
                "if there is a failure to compile the program source. This error will be returned if clCompileProgram does not return until the compile has completed."
            );
        }
        else if (err == -16) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_LINKER_NOT_AVAILABLE",
                "if a linker is not available i.e. CL_DEVICE_LINKER_AVAILABLE specified in the table of allowed values for param_name for clGetDeviceInfo is set to CL_FALSE."
            );
        }
        else if (err == -17) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_LINK_PROGRAM_FAILURE",
                "if there is a failure to link the compiled binaries and/or libraries."
            );
        }
        else if (err == -18) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_DEVICE_PARTITION_FAILED",
                "if the partition name is supported by the implementation but in_device could not be further partitioned."
            );
        }
        else if (err == -19) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_KERNEL_ARG_INFO_NOT_AVAILABLE",
                "if the argument information is not available for kernel."
            );
        }
        else if (err == -30) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_VALUE",
                "This depends on the function: two or more coupled parameters had errors."
            );
        }
        else if (err == -31) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_DEVICE_TYPE",
                "if an invalid device_type is given"
            );
        }
        else if (err == -32) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_PLATFORM",
                "if an invalid platform was given"
            );
        }
        else if (err == -33) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_DEVICE",
                "if devices contains an invalid device or are not associated with the specified platform."
            );
        }
        else if (err == -34) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_CONTEXT",
                "if context is not a valid context."
            );
        }
        else if (err == -35) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_QUEUE_PROPERTIES",
                "if specified command-queue-properties are valid but are not supported by the device."
            );
        }
        else if (err == -36) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_COMMAND_QUEUE",
                "if command_queue is not a valid command-queue."
            );
        }
        else if (err == -37) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_HOST_PTR",
                "This flag is valid only if host_ptr is not NULL. If specified, it indicates that the application wants the OpenCL implementation to allocate memory for the memory object and copy the data from memory referenced by host_ptr.CL_MEM_COPY_HOST_PTR and CL_MEM_USE_HOST_PTR are mutually exclusive.CL_MEM_COPY_HOST_PTR can be used with CL_MEM_ALLOC_HOST_PTR to initialize the contents of the cl_mem object allocated using host-accessible (e.g. PCIe) memory."
            );
        }
        else if (err == -38) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_MEM_OBJECT",
                "if memobj is not a valid OpenCL memory object."
            );
        }
        else if (err == -39) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
                "if the OpenGL/DirectX texture internal format does not map to a supported OpenCL image format."
            );
        }
        else if (err == -40) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_IMAGE_SIZE",
                "if an image object is specified as an argument value and the image dimensions (image width, height, specified or compute row and/or slice pitch) are not supported by device associated with queue."
            );
        }
        else if (err == -41) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_SAMPLER",
                "if sampler is not a valid sampler object."
            );
        }
        else if (err == -42) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_BINARY",
                "The provided binary is unfit for the selected device. if program is created with clCreateProgramWithBinary and devices listed in device_list do not have a valid program binary loaded."
            );
        }
        else if (err == -43) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_BUILD_OPTIONS",
                "if the build options specified by options are invalid."
            );
        }
        else if (err == -44) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_PROGRAM",
                "if program is a not a valid program object."
            );
        }
        else if (err == -45) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_PROGRAM_EXECUTABLE",
                "if there is no successfully built program executable available for device associated with command_queue."
            );
        }
        else if (err == -46) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_KERNEL_NAME",
                "if kernel_name is not found in program."
            );
        }
        else if (err == -47) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_KERNEL_DEFINITION",
                "if the function definition for __kernel function given by kernel_name such as the number of arguments, the argument types are not the same for all devices for which the program executable has been built."
            );
        }
        else if (err == -48) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_KERNEL",
                "if kernel is not a valid kernel object."
            );
        }
        else if (err == -49) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_ARG_INDEX",
                "if arg_index is not a valid argument index."
            );
        }
        else if (err == -50) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_ARG_VALUE",
                "if arg_value specified is not a valid value."
            );
        }
        else if (err == -51) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_ARG_SIZE",
                "if arg_size does not match the size of the data type for an argument that is not a memory object or if the argument is a memory object and arg_size != sizeof(cl_mem) or if arg_size is zero and the argument is declared with the __local qualifier or if the argument is a sampler and arg_size != sizeof(cl_sampler)."
            );
        }
        else if (err == -52) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_KERNEL_ARGS",
                "if the kernel argument values have not been specified."
            );
        }
        else if (err == -53) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_WORK_DIMENSION",
                "if work_dim is not a valid value (i.e. a value between 1 and 3)."
            );
        }
        else if (err == -54) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_WORK_GROUP_SIZE",
                "if local_work_size is specified and number of work-items specified by global_work_size is not evenly divisable by size of work-group given by local_work_size or does not match the work-group size specified for kernel using the __attribute__ ((reqd_work_group_size(X, Y, Z))) qualifier in program source.if local_work_size is specified and the total number of work-items in the work-group computed as local_work_size[0] *… local_work_size[work_dim – 1] is greater than the value specified by CL_DEVICE_MAX_WORK_GROUP_SIZE in the table of OpenCL Device Queries for clGetDeviceInfo.if local_work_size is NULL and the __attribute__ ((reqd_work_group_size(X, Y, Z))) qualifier is used to declare the work-group size for kernel in the program source."
            );
        }
        else if (err == -55) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_WORK_ITEM_SIZE",
                "if the number of work-items specified in any of local_work_size[0], … local_work_size[work_dim – 1] is greater than the corresponding values specified by CL_DEVICE_MAX_WORK_ITEM_SIZES[0], …. CL_DEVICE_MAX_WORK_ITEM_SIZES[work_dim – 1]."
            );
        }
        else if (err == -56) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_GLOBAL_OFFSET",
                "if the value specified in global_work_size + the corresponding values in global_work_offset for any dimensions is greater than the sizeof(size_t) for the device on which the kernel execution will be enqueued."
            );
        }
        else if (err == -57) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_EVENT_WAIT_LIST",
                "if event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events."
            );
        }
        else if (err == -58) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_EVENT",
                "if event objects specified in event_list are not valid event objects."
            );
        }
        else if (err == -59) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_OPERATION",
                "if interoperability is specified by setting CL_CONTEXT_ADAPTER_D3D9_KHR, CL_CONTEXT_ADAPTER_D3D9EX_KHR or CL_CONTEXT_ADAPTER_DXVA_KHR to a non-NULL value, and interoperability with another graphics API is also specified. (only if the cl_khr_dx9_media_sharing extension is supported)."
            );
        }
        else if (err == -60) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_GL_OBJECT",
                "if texture is not a GL texture object whose type matches texture_target, if the specified miplevel of texture is not defined, or if the width or height of the specified miplevel is zero."
            );
        }
        else if (err == -61) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_BUFFER_SIZE",
                "if size is 0.Implementations may return CL_INVALID_BUFFER_SIZE if size is greater than the CL_DEVICE_MAX_MEM_ALLOC_SIZE value specified in the table of allowed values for param_name for clGetDeviceInfo for all devices in context."
            );
        }
        else if (err == -62) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_MIP_LEVEL",
                "if miplevel is greater than zero and the OpenGL implementation does not support creating from non-zero mipmap levels."
            );
        }
        else if (err == -63) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_GLOBAL_WORK_SIZE",
                "if global_work_size is NULL, or if any of the values specified in global_work_size[0], …global_work_size [work_dim – 1] are 0 or exceed the range given by the sizeof(size_t) for the device on which the kernel execution will be enqueued."
            );
        }
        else if (err == -64) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_PROPERTY",
                "Vague error, depends on the function"
            );
        }
        else if (err == -65) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_IMAGE_DESCRIPTOR",
                "if values specified in image_desc are not valid or if image_desc is NULL."
            );
        }
        else if (err == -66) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_COMPILER_OPTIONS",
                "if the compiler options specified by options are invalid."
            );
        }
        else if (err == -67) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_LINKER_OPTIONS",
                "if the linker options specified by options are invalid."
            );
        }
        else if (err == -68) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_DEVICE_PARTITION_COUNT",
                "if the partition name specified in properties is CL_DEVICE_PARTITION_BY_COUNTS and the number of sub-devices requested exceeds CL_DEVICE_PARTITION_MAX_SUB_DEVICES or the total number of compute units requested exceeds CL_DEVICE_PARTITION_MAX_COMPUTE_UNITS for in_device, or the number of compute units requested for one or more sub-devices is less than zero or the number of sub-devices requested exceeds CL_DEVICE_PARTITION_MAX_COMPUTE_UNITS for in_device."
            );
        }
        else if (err == -69) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_PIPE_SIZE",
                "if pipe_packet_size is 0 or the pipe_packet_size exceeds CL_DEVICE_PIPE_MAX_PACKET_SIZE value for all devices in context or if pipe_max_packets is 0."
            );
        }
        else if (err == -70) {
            fprintf(stderr, "%s(%s) %s\n", func_name,
                "CL_INVALID_DEVICE_QUEUE",
                "when an argument is of type queue_t when it’s not a valid device queue object."
            );
        }
        else
            fprintf(stderr, "%s(%d) %s\n", func_name, err, "...");
        return 1;
    }
    return 0;
}


