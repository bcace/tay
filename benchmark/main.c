#include "tay.h"
#include "test.h"
#include "agent.h"
#include "thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>


int main() {
    tay_threads_start();

    Results *results = results_create();

    test_basic(results, MC_UNIFORM, 1000,
               0, 1, // see radius
               0, 10, // depth correction
               // TAY_CPU_SIMPLE
               // |
               TAY_CPU_GRID
               // |
               // TAY_CPU_KD_TREE
    );

    // test_nonpoint(results, 100,
    //               0, 1, // see radius
    //               0, 1, // depth correction
    //               // TAY_CPU_SIMPLE |
    //               TAY_CPU_KD_TREE |
    //               TAY_CPU_AABB_TREE
    // );

    // TaySpaceType spec_pairs[] = {
    //     // TAY_CPU_SIMPLE, TAY_CPU_SIMPLE,
    //     TAY_CPU_KD_TREE, TAY_CPU_KD_TREE,
    //     TAY_CPU_GRID, TAY_CPU_GRID,
    //     TAY_CPU_KD_TREE, TAY_CPU_GRID,
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
