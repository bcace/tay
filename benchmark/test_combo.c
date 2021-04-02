#include "test.h"
#include <stdio.h>


void test_combos(Results *results, int steps,
                 int beg_see_radius, int end_see_radius,
                 int beg_depth_correction, int end_depth_correction) {

    FILE *file;
    #if TAY_TELEMETRY
    fopen_s(&file, "test_combo_telemetry.py", "w");
    #else
    fopen_s(&file, "test_combo_runtimes.py", "w");
    #endif

    tay_log(file, "data = {\n");

    for (int i = beg_see_radius; i < end_see_radius; ++i) {
        float see_radius = SMALLEST_SEE_RADIUS * (1 << i);

        tay_log(file, "  %g: {\n", see_radius);

    //     tay_log(file, "    \"%s-%s\": [\n", space_type_name(TAY_CPU_SIMPLE), space_type_name(TAY_CPU_SIMPLE));
    //     _test(TAY_CPU_SIMPLE, TAY_CPU_SIMPLE, steps, see_radius, 0, results, file);
    //     tay_log(file, "    ],\n");

        results_reset(results);
        tay_log(file, "  },\n");
    }

    tay_log(file, "}\n");
    fclose(file);
}
