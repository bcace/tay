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

    void *grid_sort_kernels[8];
    void *grid_reset_cells; // TODO: rename to grid_unsort_kernel
} TayOcl;

typedef struct TayState TayState;
typedef struct TayGroup TayGroup;
typedef struct TayPass TayPass;

void ocl_init(TayState *state);
void ocl_destroy(TayState *state);

void ocl_on_simulation_start(TayState *state);
void ocl_on_run_start(TayState *state);
void ocl_on_run_end(TayState *state);
void ocl_on_simulation_end(TayState *state);

void ocl_simple_get_kernel(TayState *state, TayPass *pass);
void ocl_run_act_kernel(TayState *state, TayPass *pass);
void ocl_fetch_agents(TayState *state);

const char *ocl_pairing_prologue(int seer_is_point, int seen_is_point);
const char *ocl_pairing_epilogue();
const char *ocl_self_see_text(int same_group, int self_see);
const char *ocl_pairing_text(int seer_is_point, int seen_is_point, int dims);

char *ocl_get_kernel_name(TayPass *pass);

unsigned ocl_grid_add_kernel_texts(char *text, unsigned remaining_space);
void ocl_grid_run_sort_kernel(TayState *state, TayGroup *group);
void ocl_grid_run_unsort_kernel(TayState *state, TayGroup *group);
unsigned ocl_grid_add_see_kernel_text(TayPass *pass, char *text, unsigned remaining_space, int dims);
void ocl_grid_get_kernels(TayState *state);
void ocl_grid_run_see_kernel(TayOcl *ocl, TayPass *pass);

#endif
