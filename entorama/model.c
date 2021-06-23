#include "main.h"
#include "platform.h"
#include <stdio.h>


void model_load(char *path) {
    void *lib = platform_load_library(path);
    void *func = platform_load_library_function(lib, "entorama_main");
}
