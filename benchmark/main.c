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

    // test_basic(results, MC_UNIFORM, 100,
    //            0, 1, // see radius
    //            0, 1, // depth correction
    //            TAY_CPU_SIMPLE |
    //            TAY_CPU_TREE// |
    //            // TAY_CPU_GRID
    // );

    test_nonpoint(results, 100,
                  0, 1, // see radius
                  0, 1, // depth correction
                  TAY_CPU_SIMPLE |
                  TAY_CPU_TREE// |
                  // TAY_CPU_AABB_TREE
    );

    SpecPair spec_pairs[] = {
        { {TAY_CPU_SIMPLE, TAY_TRUE}, {TAY_CPU_SIMPLE, TAY_TRUE} },
        { {TAY_CPU_TREE, TAY_TRUE}, {TAY_CPU_TREE, TAY_TRUE} },
        { {TAY_CPU_GRID, TAY_TRUE}, {TAY_CPU_GRID, TAY_TRUE} },
    };

    // test_combo(results, 100,
    //             0, 1, // see radius
    //             0, 1, // depth correction
    //             spec_pairs, sizeof(spec_pairs) / sizeof(SpecPair)
    // );

    SpecPair spec_pairs_nonpoint[] = {
        { {TAY_CPU_SIMPLE, TAY_FALSE}, {TAY_CPU_SIMPLE, TAY_FALSE} },
        { {TAY_CPU_TREE, TAY_FALSE}, {TAY_CPU_TREE, TAY_FALSE} },
    };

    test_combo(results, 100,
                0, 1, // see radius
                0, 1, // depth correction
                spec_pairs_nonpoint, sizeof(spec_pairs_nonpoint) / sizeof(SpecPair)
    );

    results_destroy(results);

    tay_threads_stop();

    return 0;
}
