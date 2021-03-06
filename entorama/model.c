#include "main.h"
#include "entorama.h"
#include "platform.h"
#include <stdio.h>


void model_load(EntoramaModel *model, char *path) {

    void *lib = platform_load_library(path);
    ENTORAMA_MAIN entorama_main = platform_load_library_function(lib, "entorama_main");

    int r = entorama_main(model);
}
