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
    void *program;

    char sources[OCL_MAX_SOURCES][OCL_MAX_PATH];
    unsigned sources_count;
} TayOcl;

typedef struct TayState TayState;
typedef struct TayPass TayPass;

void ocl_init(TayState *state);
void ocl_destroy(TayState *state);

void ocl_on_simulation_start(TayState *state);
void ocl_on_run_start(TayState *state);
void ocl_on_run_end(TayState *state);
void ocl_on_simulation_end(TayState *state);

void ocl_fetch_agents(TayState *state);

const char *ocl_pairing_prologue(int seer_is_point, int seen_is_point);
const char *ocl_pairing_epilogue();
const char *ocl_self_see_text(int same_group, int self_see);
const char *ocl_pairing_text(int seer_is_point, int seen_is_point, int dims);

char *ocl_get_kernel_name(TayPass *pass);

#endif
