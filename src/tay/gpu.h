#ifndef gpu_h
#define gpu_h


typedef enum GpuBlocking {
    GPU_NON_BLOCKING = 0,
    GPU_BLOCKING = 1,
} GpuBlocking;

typedef enum GpuMemFlags {
    GPU_MEM_NONE = 0x0,
    GPU_MEM_READ_ONLY = 0x1,
    GPU_MEM_WRITE_ONLY = 0x2,
    GPU_MEM_READ_AND_WRITE = 0x3,
} GpuMemFlags;

typedef void * GpuKernel;
typedef void * GpuBuffer;

int gpu_init();
int gpu_release();
int gpu_build_program(const char *source);
GpuKernel gpu_create_kernel1(const char *name, int size1, void *arg1);
GpuKernel gpu_create_kernel2(const char *name, int size1, void *arg1, int size2, void *arg2);
GpuKernel gpu_create_kernel3(const char *name, int size1, void *arg1, int size2, void *arg2, int size3, void *arg3);
GpuKernel gpu_create_kernel4(const char *name, int size1, void *arg1, int size2, void *arg2, int size3, void *arg3, int size4, void *arg4);
GpuKernel gpu_create_kernel5(const char *name, int size1, void *arg1, int size2, void *arg2, int size3, void *arg3, int size4, void *arg4, int size5, void *arg5);
int gpu_release_kernel(GpuKernel kernel);
GpuBuffer gpu_create_buffer(GpuMemFlags device, GpuMemFlags host, int size);
int gpu_set_kernel_argument(GpuKernel kernel, GpuBuffer buffer, int size, void *arg);
int gpu_enqueue_write_buffer(GpuBuffer buffer, GpuBlocking blocking, int size, void *arg);
int gpu_enqueue_read_buffer(GpuBuffer buffer, GpuBlocking blocking, int size, void *arg);
int gpu_enqueue_kernel(GpuKernel kernel, int count);
int gpu_finish();

#endif
