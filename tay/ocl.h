#ifndef ocl_h
#define ocl_h

#define OCL_MAX_SOURCES     16
#define OCL_MAX_PATH        512


typedef struct {
    unsigned active;
    unsigned long long global_mem_size;
    unsigned long long local_mem_size;
    unsigned max_compute_units;
    unsigned max_work_item_dims;
    unsigned long long max_work_item_sizes[4];
    unsigned long long max_workgroup_size;

    void *device_id;
    void *context;
    void *queue;

    char sources[OCL_MAX_SOURCES][OCL_MAX_PATH];
    unsigned sources_count;
} TayOcl;

typedef struct TayState TayState;

void ocl_init(TayState *state);
void ocl_destroy(TayState *state);

void ocl_on_simulation_start(TayState *state);
void ocl_on_run_start(TayState *state);

void ocl_fetch_agents(TayState *state);

#endif