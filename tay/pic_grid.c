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

    int dims = 3; // TODO: fix this!!!!!!!!!!!!!

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
            for (int dim_i = 0; dim_i < dims; ++dim_i) {

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

            /* set node positions */
            if (dims == 1) {
                float4 *node_p = (float4 *)(pic->node_storage);
                for (unsigned i = 0; i < pic->node_counts.x; ++i) {
                    node_p->x = pic->origin.x + i * pic->cell_size;
                    node_p = (float4 *)((char *)node_p + pic->node_size);
                }
            }
            else if (dims == 2) {
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
            else if (dims == 3) {
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
    float4 kernel_radii = pass->radii;
    float cell_size = pic->cell_size;

    TayRange agents_range = tay_threads_range(group->space.count, task->thread_i);

    for (unsigned a_i = agents_range.beg; a_i < agents_range.end; ++a_i) {
        void *agent = pass->pic_group->storage + a_i * pass->pic_group->agent_size;
        float4 agent_p = float4_agent_position(agent);

        if (dims == 1) {}
        else if (dims == 2) {}
        else if (dims == 3) {

            int min_x = _max((int)ceilf((agent_p.x - kernel_radii.x * 1.1f - pic->origin.x) / cell_size), 0);
            int min_y = _max((int)ceilf((agent_p.y - kernel_radii.y * 1.1f - pic->origin.y) / cell_size), 0);
            int min_z = _max((int)ceilf((agent_p.z - kernel_radii.z * 1.1f - pic->origin.z) / cell_size), 0);
            int max_x = _min((int)ceilf((agent_p.x + kernel_radii.x * 1.1f - pic->origin.x) / cell_size), pic->node_counts.x);
            int max_y = _min((int)ceilf((agent_p.y + kernel_radii.y * 1.1f - pic->origin.y) / cell_size), pic->node_counts.y);
            int max_z = _min((int)ceilf((agent_p.z + kernel_radii.z * 1.1f - pic->origin.z) / cell_size), pic->node_counts.z);

            for (int z = min_z; z < max_z; ++z) {
                int z_base = z * pic->node_counts.x * pic->node_counts.y;
                for (int y = min_y; y < max_y; ++y) {
                    int y_base = y * pic->node_counts.x;
                    for (int x = min_x; x < max_x; ++x) {
                        void *node = pic->node_storage + (z_base + y_base + x) * pic->node_size;

                        pass->see(agent, node, pass->context);
                    }
                }
            }
        }
        else {}
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
