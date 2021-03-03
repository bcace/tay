#ifndef tay_const_h
#define tay_const_h

#include <stdbool.h>

#define TAY_MAX_SPACES          1
#define TAY_MAX_GROUPS          8
#define TAY_MAX_PASSES          32
#define TAY_MAX_THREADS         64
#define TAY_TELEMETRY           0
#define TAY_MAX_AGENTS          1000000
#define TAY_GPU_MAX_TEXT_SIZE   20000


typedef struct {
    union {
        struct {
            int x, y, z, w;
        };
        int arr[4];
    };
} int4;

typedef void (*TAY_SEE_FUNC)(void *, void *, void *);
typedef void (*TAY_ACT_FUNC)(void *, void *);

#endif
