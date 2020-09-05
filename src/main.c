#include "test.h"
#include "thread.h"


int main() {
    tay_runner_init();
    tay_runner_start_threads(9);
    test();
    tay_runner_stop_threads();
    return 0;
}
