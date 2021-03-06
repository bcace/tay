#ifndef gpu_h
#define gpu_h


typedef struct GpuContext GpuContext;

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

GpuContext *gpu_create(void);
int gpu_destroy(GpuContext *context);
int gpu_build_program(GpuContext *context, const char *gpu_source);
GpuKernel gpu_create_kernel(GpuContext *context, const char *name);
int gpu_set_kernel_value_argument(GpuKernel kernel, int index, void *value, int value_size);
int gpu_set_kernel_buffer_argument(GpuKernel kernel, int index, GpuBuffer *buffer);
int gpu_release_kernel(GpuKernel kernel);
GpuBuffer gpu_create_buffer(GpuContext *context, GpuMemFlags device, GpuMemFlags host, int size);
int gpu_release_buffer(GpuBuffer buffer);
int gpu_enqueue_write_buffer(GpuContext *context, GpuBuffer buffer, GpuBlocking blocking, int size, void *arg);
int gpu_enqueue_read_buffer(GpuContext *context, GpuBuffer buffer, GpuBlocking blocking, int size, void *arg);
int gpu_enqueue_kernel(GpuContext *context, GpuKernel kernel, int count);
int gpu_enqueue_kernel_nb(GpuContext *c, GpuKernel kernel, long long int *count);
int gpu_finish(GpuContext *context);

#endif
