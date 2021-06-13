#include <windows.h>


void tay_atomic_add_float(float *o, float v) {
    union {
        unsigned u32;
        float f32;
    } old, new;
    do  {
        old.u32 = InterlockedExchangeAdd((unsigned *)o, 0);
        new.f32 = old.f32 + v;
    }
    while (old.u32 != InterlockedCompareExchange((unsigned *)o, new.u32, old.u32));
}
