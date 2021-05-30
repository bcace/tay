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

#if 0
    test_basic(results, MC_UNIFORM, 100,
               0, 1, // see radius
               0, 1, // depth correction
               // TAY_CPU_SIMPLE
               // |
               TAY_CPU_GRID
               |
               // TAY_CPU_Z_GRID
               // |
               // TAY_CPU_KD_TREE
               // |
               TAY_OCL_SIMPLE
               |
               TAY_OCL_GRID
    );
#endif

#if 0
    test_nonpoint(results, 100,
                  0, 1, // see radius
                  0, 1, // depth correction
                  // TAY_CPU_SIMPLE
                  // |
                  // TAY_CPU_KD_TREE
                  // |
                  TAY_CPU_AABB_TREE
                  |
                  TAY_OCL_SIMPLE
    );
#endif

#if 1
    TaySpaceType spec_pairs[] = {
        // TAY_CPU_SIMPLE, TAY_CPU_SIMPLE,
        // TAY_CPU_KD_TREE, TAY_CPU_KD_TREE,
        TAY_CPU_GRID, TAY_CPU_GRID,
        // TAY_CPU_KD_TREE, TAY_CPU_GRID,
        // TAY_OCL_SIMPLE, TAY_OCL_SIMPLE,
        // TAY_OCL_GRID, TAY_OCL_GRID,
        TAY_OCL_GRID, TAY_OCL_SIMPLE,
        TAY_SPACE_NONE,
    };

    test_combo(results, 100, TAY_TRUE, TAY_TRUE,
                0, 1, // see radius
                0, 1, // depth correction
                spec_pairs
    );
#endif

#if 0
    TaySpaceType spec_pairs_nonpoint[] = {
        // TAY_CPU_SIMPLE, TAY_CPU_SIMPLE,
        TAY_CPU_KD_TREE, TAY_CPU_KD_TREE,
        TAY_CPU_AABB_TREE, TAY_CPU_AABB_TREE,
        TAY_OCL_SIMPLE, TAY_OCL_SIMPLE,
        TAY_SPACE_NONE,
    };

    test_combo(results, 100, TAY_FALSE, TAY_FALSE,
                0, 1, // see radius
                0, 1, // depth correction
                spec_pairs_nonpoint
    );
#endif

    results_destroy(results);

    tay_threads_stop();

    return 0;
}
