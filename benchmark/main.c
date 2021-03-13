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

    test_basic(results, 100, 0, 2, 0, 3, MC_UNIFORM);
    // test_nonpoint(results, 100, 0, 2, 0, 3);

    results_destroy(results);

    tay_threads_stop();

    return 0;
}
