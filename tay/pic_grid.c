#include "state.h"
#include <float.h>


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
int pic_prepare_grids(TayState *state, int dims) {

    /* if there are no pics there's nothing to prepare */
    if (state->pics_count == 0)
        return 0;

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
        TayPicGrid *pic;
        TayGroup *group;

        if (pass->pic_type == TAY_PIC_SEER) {
            pic = pass->seer_pic;
            group = pass->seen_group;
        }
        else if (pass->pic_type == TAY_PIC_SEEN) {
            pic = pass->seen_pic;
            group = pass->seer_group;
        }
        else
            continue;

        /* since simple spaces don't update their box as part of their regular functioning we have to do it here */
        if (group->space.type == TAY_CPU_SIMPLE && !simple_spaces_boxes_updated[group - state->groups]) {
            space_update_box(group);
            simple_spaces_boxes_updated[group - state->groups] = 1;
        }

        _include_box(pic_boxes + (pic - state->pics), &group->space.box);
        pic->nodes_count = 1; /* mark pic as active since it interacts with a group, keep it 1 because there's *= later */
    }

    /* active grids (nodes_count set to 1) calculate their origins and node counts */
    for (unsigned pic_i = 0; pic_i < state->pics_count; ++pic_i) {
        TayPicGrid *pic = state->pics + pic_i;

        if (pic_is_active(pic)) {

            for (int dim_i = 0; dim_i > dims; ++dim_i) {

                float box_side = pic_boxes[pic_i].max.arr[dim_i] - pic_boxes[pic_i].min.arr[dim_i];
                unsigned count = (unsigned)ceil(box_side / pic->cell_sizes.arr[dim_i]);
                float grid_side = count * pic->cell_sizes.arr[dim_i];
                float margin = (grid_side - box_side) * 0.5f;

                pic->nodes_count *= count;
                pic->node_counts.arr[dim_i] = count;
                pic->origin.arr[dim_i] = pic_boxes[pic_i].min.arr[dim_i] - margin;
            }
        }
    }

    return 1;
}
