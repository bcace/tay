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
    //            0, 2, /* see radius */
    //            0, 3, /* depth correction */
    //            TAY_CPU_SIMPLE | TAY_CPU_TREE | TAY_CPU_GRID
    // );

    test_nonpoint(results, 100,
                  0, 2, /* see radius */
                  0, 2, /* depth correction */
                  // TAY_CPU_SIMPLE |
                  TAY_CPU_TREE |
                  TAY_CPU_AABB_TREE
    );

    // test_point_nonpoint_combo(results, 100,
    //                           0, 2, /* see radius */
    //                           0, 1, /* depth correction */
    //                           TAY_CPU_SIMPLE |
    //                           TAY_CPU_TREE |
    //                           TAY_CPU_GRID
    //                           ,
    //                           TAY_CPU_SIMPLE |
    //                           TAY_CPU_TREE |
    //                           TAY_CPU_AABB_TREE
    // );

    results_destroy(results);

    tay_threads_stop();

    return 0;
}
