#include "vec.h"
#include <stdlib.h>
#include <math.h>


vec vec_null() {
    vec r;
    r.x = r.y = r.z = 0.0f;
    return r;
}

vec vec_make(float x, float y, float z) {
    vec r;
    r.x = x;
    r.y = y;
    r.z = z;
    return r;
}

vec vec_rand_scalar(float min, float max) {
    vec r;
    float range = max - min;
    r.x = min + rand() * range / (float)RAND_MAX;
    r.y = min + rand() * range / (float)RAND_MAX;
    r.z = min + rand() * range / (float)RAND_MAX;
    return r;
}

vec vec_rand(vec min, vec max) {
    vec r;
    r.x = min.x + rand() * (max.x - min.x) / (float)RAND_MAX;
    r.y = min.y + rand() * (max.y - min.y) / (float)RAND_MAX;
    r.z = min.z + rand() * (max.z - min.z) / (float)RAND_MAX;
    return r;
}

vec vec_sub(vec a, vec b) {
    vec r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

vec vec_add(vec a, vec b) {
    vec r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

float vec_length(vec a) {
    return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

vec vec_mul_scalar(vec a, float s) {
    vec r;
    r.x = a.x * s;
    r.y = a.y * s;
    r.z = a.z * s;
    return r;
}

vec vec_div_scalar(vec a, float s) {
    vec r;
    r.x = a.x / s;
    r.y = a.y / s;
    r.z = a.z / s;
    return r;
}

vec vec_normalize(vec a) {
    return vec_div_scalar(a, vec_length(a));
}

vec vec_normalize_to(vec a, float s) {
    return vec_mul_scalar(a, s / vec_length(a));
}
