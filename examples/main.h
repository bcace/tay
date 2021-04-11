#ifndef main_h
#define main_h

#include <stdbool.h>


typedef enum {
    FLOCKING,
    FLUID,
} Examples;

typedef struct Global {
    Examples example;
    bool window_quit;
    int window_w;
    int window_h;
    struct TayState *tay;
    struct vec3 *inst_vec3_buffers[2];
    float *inst_float_buffers[1];
    int max_agents_count;
} Global;

Global global;

void flocking_init();
void flocking_draw();

void fluid_init();
void fluid_draw();

#endif