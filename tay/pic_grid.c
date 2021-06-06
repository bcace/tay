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

static unsigned _group_index(TayGroup *groups, TayGroup *group) {
    return (unsigned)(group - groups);
}

/* NOTE: this must be called at the beginning of each step and after agent groups
have updated their boxes. */
int pic_prepare_grids(TayState *state) {
    if (state->pics_count == 0)
        return 0;

    /* reset pic grid boxes */
    for (unsigned pic_i = 0; pic_i < state->pics_count; ++pic_i) {
        TayPicGrid *pic = state->pics + pic_i;
        pic->box.min = (float4){FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
        pic->box.max = (float4){-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};
        pic->nodes_count = 0;
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

        /* since simple doesn't update its box as part of its regular functioning we have to do it here */
        if (group->space.type == TAY_CPU_SIMPLE && !simple_spaces_boxes_updated[group - state->groups]) {
            space_update_box(group);
            simple_spaces_boxes_updated[group - state->groups] = 1;
        }

        _include_box(&pic->box, &group->space.box);
        pic->nodes_count = 1;
    }

    /* active grids (nodes_count set to 1) calculate their origins, cell sizes and node counts */
    for (unsigned pic_i = 0; pic_i < state->pics_count; ++pic_i) {
        TayPicGrid *pic = state->pics + pic_i;

        if (pic->nodes_count) {
            // ...
        }
    }

    return 1;
}
