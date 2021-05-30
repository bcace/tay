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

typedef struct {
    char *text;
    unsigned length;
    unsigned max_length;
} OclText;

typedef struct TayState TayState;
typedef struct TayGroup TayGroup;
typedef struct TayPass TayPass;

void ocl_init(TayState *state);
void ocl_destroy(TayState *state);

void ocl_on_simulation_start(TayState *state);
void ocl_on_run_start(TayState *state);
void ocl_on_run_end(TayState *state);
void ocl_on_simulation_end(TayState *state);

void ocl_run_act_kernel(TayState *state, TayPass *pass);
void ocl_fetch_agents(TayState *state);

char *ocl_get_kernel_name(TayPass *pass);

void ocl_add_seen_text(OclText *text, TayPass *pass, int dims);

void ocl_simple_add_seen_text(OclText *text, TayPass *pass, int dims);
void ocl_simple_add_see_kernel_text(OclText *text, TayPass *pass, int dims);
void ocl_simple_run_see_kernel(TayOcl *ocl, TayPass *pass);

void ocl_grid_add_kernel_texts(OclText *text);
void ocl_grid_run_sort_kernel(TayState *state, TayGroup *group);
void ocl_grid_run_unsort_kernel(TayState *state, TayGroup *group);
void ocl_grid_add_seen_text(OclText *text, TayPass *pass, int dims);
void ocl_grid_add_see_kernel_text(OclText *text, TayPass *pass, int dims);
void ocl_grid_get_kernels(TayState *state);
void ocl_grid_run_see_kernel(TayOcl *ocl, TayPass *pass);

/* ocl_generics.c */
const char *ocl_get_seer_agent_text(TayPass *pass);
char *ocl_get_coupling_text(TayPass *pass, int dims);

void ocl_text_init(OclText *text, unsigned max_length);
void ocl_text_append(OclText *text, char *fmt, ...);
char *ocl_text_reserve(OclText *text, unsigned length);
void ocl_text_add_end_of_string(OclText *text);
void ocl_text_destroy(OclText *text);

#endif
