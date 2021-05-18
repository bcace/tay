#include "state.h"
#include "CL/cl.h"
#include <stdio.h>


unsigned ocl_grid_add_sort_kernel_text(TayPass *pass, char *text, unsigned remaining_space, int dims) {
    #ifdef TAY_OCL

    unsigned length = sprintf_s(text, remaining_space, "\n\
\n");

    return length;

    #else
    return 0u;
    #endif
}
