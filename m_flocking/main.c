#include "entorama.h"


__declspec(dllexport) int entorama_main(EntoramaModelInfo *info) {
    info->dummy = 33;
    return 13;
}
