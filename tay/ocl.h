#ifndef ocl_h
#define ocl_h


typedef struct {
    unsigned active;
    void *device_id;
    unsigned long long global_mem_size;
    unsigned long long local_mem_size;
    unsigned max_compute_units;
    unsigned max_work_item_dims;
    unsigned long long max_work_item_sizes[4];
    unsigned long long max_workgroup_size;
} TayOcl;

void ocl_init(TayOcl *ocl);

#endif
