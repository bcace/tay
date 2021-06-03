#include "tay.h"
#include "test.h"
#include "agent_host.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>


int main() {
    tay_threads_start(100000);

    Results *results = results_create();

    Configs configs;

#if 1
    spaces_init(&configs);
    space_add_single(&configs, TAY_CPU_SIMPLE, 0);
    space_add_single(&configs, TAY_CPU_KD_TREE, 0);
    space_add_single(&configs, TAY_CPU_GRID, 0);
    space_add_single(&configs, TAY_CPU_Z_GRID, 0);
    space_add_single(&configs, TAY_CPU_SIMPLE, 1);
    space_add_single(&configs, TAY_CPU_GRID, 1);

    test_basic(results, MC_UNIFORM, 100,
               0, 1, // see radius
               0, 1, // depth correction
               &configs);
#endif

#if 1
    spaces_init(&configs);
    space_add_single(&configs, TAY_CPU_SIMPLE, 0);
    space_add_single(&configs, TAY_CPU_KD_TREE, 0);
    space_add_single(&configs, TAY_CPU_AABB_TREE, 0);
    space_add_single(&configs, TAY_CPU_SIMPLE, 1);

    test_nonpoint(results, 100,
                  0, 1, // see radius
                  0, 1, // depth correction
                  &configs);
#endif

#if 1
    spaces_init(&configs);
    space_add_double(&configs, TAY_CPU_SIMPLE, 0, TAY_CPU_SIMPLE, 0);
    space_add_double(&configs, TAY_CPU_KD_TREE, 0, TAY_CPU_KD_TREE, 0);
    space_add_double(&configs, TAY_CPU_GRID, 0, TAY_CPU_GRID, 0);
    space_add_double(&configs, TAY_CPU_KD_TREE, 0, TAY_CPU_GRID, 0);
    space_add_double(&configs, TAY_CPU_SIMPLE, 1, TAY_CPU_SIMPLE, 1);
    space_add_double(&configs, TAY_CPU_GRID, 1, TAY_CPU_GRID, 1);
    space_add_double(&configs, TAY_CPU_GRID, 1, TAY_CPU_SIMPLE, 1);

    test_combo(results, 100, TAY_TRUE, TAY_TRUE,
               0, 1, // see radius
               0, 1, // depth correction
               &configs);
#endif

#if 1
    spaces_init(&configs);
    space_add_double(&configs, TAY_CPU_SIMPLE, 0, TAY_CPU_SIMPLE, 0);
    space_add_double(&configs, TAY_CPU_KD_TREE, 0, TAY_CPU_KD_TREE, 0);
    space_add_double(&configs, TAY_CPU_AABB_TREE, 0, TAY_CPU_AABB_TREE, 0);
    space_add_double(&configs, TAY_CPU_SIMPLE, 1, TAY_CPU_SIMPLE, 1);

    test_combo(results, 100, TAY_FALSE, TAY_FALSE,
                0, 1, // see radius
                0, 1, // depth correction
                &configs);
#endif

    results_destroy(results);

    tay_threads_stop();

    return 0;
}
