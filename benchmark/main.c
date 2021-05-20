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

    test_basic(results, MC_UNIFORM, 10000,
               0, 1, // see radius
               0, 1, // depth correction
               // TAY_CPU_SIMPLE
               // |
               // TAY_CPU_GRID
               // |
               // TAY_CPU_Z_GRID
               // |
               // TAY_CPU_KD_TREE
               // |
               // TAY_OCL_SIMPLE
               // |
               TAY_OCL_GRID
    );

    // test_nonpoint(results, 100,
    //               0, 1, // see radius
    //               0, 1, // depth correction
    //               // TAY_CPU_SIMPLE
    //               // |
    //               // TAY_CPU_KD_TREE
    //               // |
    //               TAY_CPU_AABB_TREE
    //               |
    //               TAY_OCL_SIMPLE
    // );

    // TaySpaceType spec_pairs[] = {
    //     // TAY_CPU_SIMPLE, TAY_CPU_SIMPLE,
    //     TAY_CPU_KD_TREE, TAY_CPU_KD_TREE,
    //     TAY_CPU_GRID, TAY_CPU_GRID,
    //     TAY_CPU_KD_TREE, TAY_CPU_GRID,
    //     TAY_OCL_SIMPLE, TAY_OCL_SIMPLE,
    //     TAY_SPACE_NONE,
    // };

    // test_combo(results, 100, TAY_TRUE, TAY_TRUE,
    //             0, 1, // see radius
    //             0, 1, // depth correction
    //             spec_pairs
    // );

    // TaySpaceType spec_pairs_nonpoint[] = {
    //     // TAY_CPU_SIMPLE, TAY_CPU_SIMPLE,
    //     TAY_CPU_KD_TREE, TAY_CPU_KD_TREE,
    //     TAY_CPU_AABB_TREE, TAY_CPU_AABB_TREE,
    //     TAY_OCL_SIMPLE, TAY_OCL_SIMPLE,
    //     TAY_SPACE_NONE,
    // };

    // test_combo(results, 100, TAY_FALSE, TAY_FALSE,
    //             0, 1, // see radius
    //             0, 1, // depth correction
    //             spec_pairs_nonpoint
    // );

    results_destroy(results);

    tay_threads_stop();

    return 0;
}
