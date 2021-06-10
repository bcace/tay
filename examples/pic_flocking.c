#include "main.h"
#include "tay.h"
#include "agent_host.h"


static int boids_count = 8000;
static float pic_cell_size = 1.0f;
static float pic_transfer_r = 10.0f;

static TayGroup *boids_group;
static TayPicGrid *pic;

static PicFlockingContext context;

void pic_flocking_init() {
    TayState *tay = demos.tay;

    boids_group = tay_add_group(tay, sizeof(PicBoid), boids_count, TAY_TRUE);
    pic = tay_add_pic_grid(tay, sizeof(PicBoidNode), 1000000, pic_cell_size);

    context.radius = pic_transfer_r;

    tay_add_pic_act(tay, pic, pic_reset_node, 0);
    tay_add_pic_see(tay, boids_group, pic, pic_transfer_boid_to_node, (float4){pic_transfer_r, pic_transfer_r, pic_transfer_r, pic_transfer_r}, &context);
}
