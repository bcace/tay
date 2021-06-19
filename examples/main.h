#ifndef main_h
#define main_h

#include <stdbool.h>


typedef enum {
    FLOCKING,
    PIC_FLOCKING,
    FLUID,
    TAICHI_2D,
} Examples;

typedef struct Global {
    Examples example;
    bool window_quit;
    int window_w;
    int window_h;
    struct TayState *tay;
    struct vec3 *inst_vec3_buffers[2];
    float *inst_float_buffers[2];
    int max_agents_count;
} Global;

Global demos;

void flocking_init();
void flocking_draw();

void pic_flocking_init();
void pic_flocking_draw();

void fluid_init();
void fluid_draw();

void taichi_2D_init();
void taichi_2D_draw();

float PYRAMID_VERTS[];
int PYRAMID_VERTS_COUNT;

float ICOSAHEDRON_VERTS[];
int ICOSAHEDRON_VERTS_COUNT;

float CUBE_VERTS[];
int CUBE_VERTS_COUNT;

int icosahedron_verts_count(unsigned subdivs);
void icosahedron_verts(unsigned subdivs, float *verts);

#endif
