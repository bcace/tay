#include "tay.h"
#include "test.h"
#include "agent_host.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>


int main() {
    tay_threads_start(0, 100000);

    Results *results = results_create();

    Configs configs;

#if 1
    spaces_init(&configs);
    space_add_single(&configs, TAY_CPU_SIMPLE);
    space_add_single(&configs, TAY_CPU_KD_TREE);
    space_add_single(&configs, TAY_CPU_GRID);
    space_add_single(&configs, TAY_CPU_Z_GRID);
    space_add_single(&configs, TAY_OCL_SIMPLE);
    space_add_single(&configs, TAY_OCL_Z_GRID);

    test_basic(results, MC_UNIFORM, 100,
               0, 1, // see radius
               0, 1, // depth correction
               &configs);
#endif

#if 1
    spaces_init(&configs);
    space_add_single(&configs, TAY_CPU_SIMPLE);
    space_add_single(&configs, TAY_CPU_KD_TREE);
    space_add_single(&configs, TAY_CPU_AABB_TREE);
    space_add_single(&configs, TAY_OCL_SIMPLE);

    test_nonpoint(results, 100,
                  0, 1, // see radius
                  0, 1, // depth correction
                  &configs);
#endif

#if 1
    spaces_init(&configs);
    space_add_double(&configs, TAY_CPU_SIMPLE, TAY_CPU_SIMPLE);
    space_add_double(&configs, TAY_CPU_KD_TREE, TAY_CPU_KD_TREE);
    space_add_double(&configs, TAY_CPU_GRID, TAY_CPU_GRID);
    space_add_double(&configs, TAY_CPU_KD_TREE, TAY_CPU_GRID);
    space_add_double(&configs, TAY_OCL_SIMPLE, TAY_OCL_SIMPLE);
    space_add_double(&configs, TAY_OCL_Z_GRID, TAY_OCL_Z_GRID);
    space_add_double(&configs, TAY_OCL_Z_GRID, TAY_OCL_SIMPLE);

    test_combo(results, 100, TAY_TRUE, TAY_TRUE,
               0, 1, // see radius
               0, 1, // depth correction
               &configs);
#endif

#if 1
    spaces_init(&configs);
    space_add_double(&configs, TAY_CPU_SIMPLE, TAY_CPU_SIMPLE);
    space_add_double(&configs, TAY_CPU_KD_TREE, TAY_CPU_KD_TREE);
    space_add_double(&configs, TAY_CPU_AABB_TREE, TAY_CPU_AABB_TREE);
    space_add_double(&configs, TAY_OCL_SIMPLE, TAY_OCL_SIMPLE);

    test_combo(results, 100, TAY_FALSE, TAY_FALSE,
                0, 1, // see radius
                0, 1, // depth correction
                &configs);
#endif

    results_destroy(results);

    tay_threads_stop();

    return 0;
}
