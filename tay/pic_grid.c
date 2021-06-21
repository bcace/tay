#include "space.h"
#include "thread.h"
#include <float.h>
#include <math.h>


static inline void _include_box(Box *a, Box *b) {
    if (b->min.x < a->min.x)
        a->min.x = b->min.x;
    if (b->min.y < a->min.y)
        a->min.y = b->min.y;
    if (b->min.z < a->min.z)
        a->min.z = b->min.z;
    if (b->min.w < a->min.w)
        a->min.w = b->min.w;
    if (b->max.x > a->max.x)
        a->max.x = b->max.x;
    if (b->max.y > a->max.y)
        a->max.y = b->max.y;
    if (b->max.z > a->max.z)
        a->max.z = b->max.z;
    if (b->max.w > a->max.w)
        a->max.w = b->max.w;
}

/* NOTE: this must be called at the beginning of each step and after agent groups
have updated their boxes. */
int pic_prepare_grids(TayState *state) {

    /* if there are no pics there's nothing to prepare */
    if (state->pics_count == 0)
        return 1;

    Box pic_boxes[TAY_MAX_PICS];

    /* reset pic boxes */
    for (unsigned pic_i = 0; pic_i < state->pics_count; ++pic_i) {
        TayPicGrid *pic = state->pics + pic_i;

        pic_boxes[pic_i].min = (float4){FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
        pic_boxes[pic_i].max = (float4){-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};

        pic->nodes_count = 0; /* mark pic as not active yet */
    }

    int simple_spaces_boxes_updated[TAY_MAX_GROUPS] = {0};

    /* update pic grid boxes with boxes of agent spaces they interact with */
    for (unsigned pass_i = 0; pass_i < state->passes_count; ++pass_i) {
        TayPass *pass = state->passes + pass_i;

        if (!pass->is_pic || pass->type != TAY_PASS_SEE)
            continue;

        TayPicGrid *pic = pass->pic;
        TayGroup *group = pass->pic_group;

        /* since simple spaces don't update their box as part of their regular functioning we have to do it here */
        if (group->space.type == TAY_CPU_SIMPLE && !simple_spaces_boxes_updated[group - state->groups]) {
            space_update_box(group);
            simple_spaces_boxes_updated[group - state->groups] = 1;
        }

        _include_box(pic_boxes + (pic - state->pics), &group->space.box);
        pic->nodes_count = 1; /* mark pic as active since it interacts with a group, keep it 1 because there's *= later */
    }

    /* calculate their origins and node counts for active grids (nodes_count set to 1) */
    for (unsigned pic_i = 0; pic_i < state->pics_count; ++pic_i) {
        TayPicGrid *pic = state->pics + pic_i;

        if (pic_is_active(pic)) {

            /* calculate origin and node counts */
            for (int dim_i = 0; dim_i < pic->dims; ++dim_i) {

                float box_side = pic_boxes[pic_i].max.arr[dim_i] - pic_boxes[pic_i].min.arr[dim_i];
                unsigned count = (unsigned)ceilf(box_side * 1.001f / pic->cell_size);
                float grid_side = count * pic->cell_size;
                float margin = (grid_side - box_side) * 0.5f;

                pic->nodes_count *= count;
                pic->node_counts.arr[dim_i] = count;
                pic->origin.arr[dim_i] = pic_boxes[pic_i].min.arr[dim_i] - margin;
            }

            /* check if the storage is large enough to store all nodes */
            if (pic->nodes_count > pic->nodes_capacity) {
                tay_set_error2(state, TAY_ERROR_PIC, "not enough storage allocated for required number of pic grid nodes");
                return 0;
            }

            // TODO: do this only if grid changed

            /* set node positions */
            if (pic->dims == 1) {
                float4 *node_p = (float4 *)(pic->node_storage);
                for (unsigned i = 0; i < pic->node_counts.x; ++i) {
                    node_p->x = pic->origin.x + i * pic->cell_size;
                    node_p = (float4 *)((char *)node_p + pic->node_size);
                }
            }
            else if (pic->dims == 2) {
                float4 *node_p = (float4 *)(pic->node_storage);
                for (unsigned j = 0; j < pic->node_counts.y; ++j) {
                    float y = pic->origin.y + j * pic->cell_size;
                    for (unsigned i = 0; i < pic->node_counts.x; ++i) {
                        node_p->x = pic->origin.x + i * pic->cell_size;
                        node_p->y = y;
                        node_p = (float4 *)((char *)node_p + pic->node_size);
                    }
                }
            }
            else if (pic->dims == 3) {
                float4 *node_p = (float4 *)(pic->node_storage);
                for (unsigned k = 0; k < pic->node_counts.z; ++k) {
                    float z = pic->origin.z + k * pic->cell_size;
                    for (unsigned j = 0; j < pic->node_counts.y; ++j) {
                        float y = pic->origin.y + j * pic->cell_size;
                        for (unsigned i = 0; i < pic->node_counts.x; ++i) {
                            node_p->x = pic->origin.x + i * pic->cell_size;
                            node_p->y = y;
                            node_p->z = z;
                            node_p = (float4 *)((char *)node_p + pic->node_size);
                        }
                    }
                }
            }
            else {
                float4 *node_p = (float4 *)(pic->node_storage);
                for (unsigned l = 0; l < pic->node_counts.w; ++l) {
                    float w = pic->origin.w + l * pic->cell_size;
                    for (unsigned k = 0; k < pic->node_counts.z; ++k) {
                        float z = pic->origin.z + k * pic->cell_size;
                        for (unsigned j = 0; j < pic->node_counts.y; ++j) {
                            float y = pic->origin.y + j * pic->cell_size;
                            for (unsigned i = 0; i < pic->node_counts.x; ++i) {
                                node_p->x = pic->origin.x + i * pic->cell_size;
                                node_p->y = y;
                                node_p->z = z;
                                node_p->w = w;
                                node_p = (float4 *)((char *)node_p + pic->node_size);
                            }
                        }
                    }
                }
            }
        }
    }

    return 1;
}

static inline int _min(int a, int b) {
    return (a < b) ? a : b;
}

static inline int _max(int a, int b) {
    return (a > b) ? a : b;
}

static void _see_func(TayThreadTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    TayPicGrid *pic = pass->pic;
    TayGroup *group = pass->pic_group;

    int dims = group->space.dims;
    unsigned kernel_size = pass->kernel_size;
    float base_offset = (kernel_size - 2) * 0.5f;
    float inv_cell_size = 1.0f / pic->cell_size;

    TayRange agents_range = tay_threads_range(group->space.count, task->thread_i);

    TayPicKernel kernel;
    kernel.cell_size = pic->cell_size;
    kernel.nodes = thread_context->storage;

    for (unsigned a_i = agents_range.beg; a_i < agents_range.end; ++a_i) {
        void *agent = pass->pic_group->storage + a_i * pass->pic_group->agent_size;
        float4 p = float4_agent_position(agent);

        if (dims == 1) {

            /* calculate min and max pic grid node indices [min, max> */
            int min_x = (int)floorf((p.x - pic->origin.x) * inv_cell_size - base_offset);
            int max_x = min_x + kernel_size;

            /* clamp pic grid node indices [0, count> */
            if (min_x < 0)
                min_x = 0;
            if (max_x > (int)pic->node_counts.x)
                max_x = pic->node_counts.x;

            kernel.sizes.x = max_x - min_x;

            int node_i = 0;
            for (int x = min_x; x < max_x; ++x) {
                kernel.nodes[node_i++] = pic->node_storage + x * pic->node_size;
            }

            pass->see(agent, &kernel, pass->context);
        }
        else if (dims == 2) {

            /* calculate min and max pic grid node indices [min, max> */
            int min_x = (int)floorf((p.x - pic->origin.x) * inv_cell_size - base_offset);
            int min_y = (int)floorf((p.y - pic->origin.y) * inv_cell_size - base_offset);
            int max_x = min_x + kernel_size;
            int max_y = min_y + kernel_size;

            /* clamp pic grid node indices [0, count> */
            if (min_x < 0)
                min_x = 0;
            if (min_y < 0)
                min_y = 0;
            if (max_x > (int)pic->node_counts.x)
                max_x = pic->node_counts.x;
            if (max_y > (int)pic->node_counts.y)
                max_y = pic->node_counts.y;

            kernel.sizes.x = max_x - min_x;
            kernel.sizes.y = max_y - min_y;

            int node_i = 0;
            for (int y = min_y; y < max_y; ++y) {
                int y_base = y * pic->node_counts.x;
                for (int x = min_x; x < max_x; ++x) {
                    kernel.nodes[node_i++] = pic->node_storage + (y_base + x) * pic->node_size;
                }
            }

            pass->see(agent, &kernel, pass->context);
        }
        else if (dims == 3) {

            /* calculate min and max pic grid node indices [min, max> */
            int min_x = (int)floorf((p.x - pic->origin.x) * inv_cell_size - base_offset);
            int min_y = (int)floorf((p.y - pic->origin.y) * inv_cell_size - base_offset);
            int min_z = (int)floorf((p.z - pic->origin.z) * inv_cell_size - base_offset);
            int max_x = min_x + kernel_size;
            int max_y = min_y + kernel_size;
            int max_z = min_z + kernel_size;

            /* clamp pic grid node indices [0, count> */
            if (min_x < 0)
                min_x = 0;
            if (min_y < 0)
                min_y = 0;
            if (min_z < 0)
                min_z = 0;
            if (max_x > (int)pic->node_counts.x)
                max_x = pic->node_counts.x;
            if (max_y > (int)pic->node_counts.y)
                max_y = pic->node_counts.y;
            if (max_z > (int)pic->node_counts.z)
                max_z = pic->node_counts.z;

            kernel.sizes.x = max_x - min_x;
            kernel.sizes.y = max_y - min_y;
            kernel.sizes.z = max_z - min_z;

            int node_i = 0;
            for (int z = min_z; z < max_z; ++z) {
                int z_base = z * pic->node_counts.x * pic->node_counts.y;
                for (int y = min_y; y < max_y; ++y) {
                    int y_base = y * pic->node_counts.x;
                    for (int x = min_x; x < max_x; ++x) {
                        kernel.nodes[node_i++] = pic->node_storage + (z_base + y_base + x) * pic->node_size;
                    }
                }
            }

            pass->see(agent, &kernel, pass->context);
        }
        else {

            /* calculate min and max pic grid node indices [min, max> */
            int min_x = (int)floorf((p.x - pic->origin.x) * inv_cell_size - base_offset);
            int min_y = (int)floorf((p.y - pic->origin.y) * inv_cell_size - base_offset);
            int min_z = (int)floorf((p.z - pic->origin.z) * inv_cell_size - base_offset);
            int min_w = (int)floorf((p.w - pic->origin.w) * inv_cell_size - base_offset);
            int max_x = min_x + kernel_size;
            int max_y = min_y + kernel_size;
            int max_z = min_z + kernel_size;
            int max_w = min_w + kernel_size;

            /* clamp pic grid node indices [0, count> */
            if (min_x < 0)
                min_x = 0;
            if (min_y < 0)
                min_y = 0;
            if (min_z < 0)
                min_z = 0;
            if (min_w < 0)
                min_w = 0;
            if (max_x > (int)pic->node_counts.x)
                max_x = pic->node_counts.x;
            if (max_y > (int)pic->node_counts.y)
                max_y = pic->node_counts.y;
            if (max_z > (int)pic->node_counts.z)
                max_z = pic->node_counts.z;
            if (max_w > (int)pic->node_counts.w)
                max_w = pic->node_counts.w;

            kernel.sizes.x = max_x - min_x;
            kernel.sizes.y = max_y - min_y;
            kernel.sizes.z = max_z - min_z;
            kernel.sizes.w = max_w - min_w;

            int node_i = 0;
            for (int w = min_w; w < max_w; ++w) {
                int w_base = w * pic->node_counts.x * pic->node_counts.y * pic->node_counts.z;
                for (int z = min_z; z < max_z; ++z) {
                    int z_base = z * pic->node_counts.x * pic->node_counts.y;
                    for (int y = min_y; y < max_y; ++y) {
                        int y_base = y * pic->node_counts.x;
                        for (int x = min_x; x < max_x; ++x) {
                            kernel.nodes[node_i++] = pic->node_storage + (w_base + z_base + y_base + x) * pic->node_size;
                        }
                    }
                }
            }

            pass->see(agent, &kernel, pass->context);
        }
    }
}

void pic_run_see_pass(TayPass *pass) {
    space_run_thread_tasks(pass, _see_func);
}

static void _act_func(TayThreadTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    TayPicGrid *pic = pass->pic;

    TayRange nodes_range = tay_threads_range(pic->nodes_count, task->thread_i);

    for (unsigned node_i = nodes_range.beg; node_i < nodes_range.end; ++node_i) {
        void *node = pic->node_storage + node_i * pic->node_size;
        pass->act(node, pass->context);
    }
}

void pic_run_act_pass(TayPass *pass) {
    space_run_thread_tasks(pass, _act_func);
}
