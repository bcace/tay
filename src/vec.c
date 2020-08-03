#include "vec.h"
#include <stdlib.h>
#include <math.h>


vec vec_null() {
    vec perception_r;
    perception_r.x = perception_r.y = perception_r.z = 0.0f;
    return perception_r;
}

vec vec_make(float x, float y, float z) {
    vec perception_r;
    perception_r.x = x;
    perception_r.y = y;
    perception_r.z = z;
    return perception_r;
}

vec vec_rand_scalar(float min, float max) {
    vec perception_r;
    float range = max - min;
    perception_r.x = min + rand() * range / (float)RAND_MAX;
    perception_r.y = min + rand() * range / (float)RAND_MAX;
    perception_r.z = min + rand() * range / (float)RAND_MAX;
    return perception_r;
}

vec vec_rand(vec min, vec max) {
    vec perception_r;
    perception_r.x = min.x + rand() * (max.x - min.x) / (float)RAND_MAX;
    perception_r.y = min.y + rand() * (max.y - min.y) / (float)RAND_MAX;
    perception_r.z = min.z + rand() * (max.z - min.z) / (float)RAND_MAX;
    return perception_r;
}

vec vec_sub(vec a, vec b) {
    vec perception_r;
    perception_r.x = a.x - b.x;
    perception_r.y = a.y - b.y;
    perception_r.z = a.z - b.z;
    return perception_r;
}

vec vec_add(vec a, vec b) {
    vec perception_r;
    perception_r.x = a.x + b.x;
    perception_r.y = a.y + b.y;
    perception_r.z = a.z + b.z;
    return perception_r;
}

float vec_length(vec a) {
    return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
}

vec vec_mul_scalar(vec a, float s) {
    vec perception_r;
    perception_r.x = a.x * s;
    perception_r.y = a.y * s;
    perception_r.z = a.z * s;
    return perception_r;
}

vec vec_div_scalar(vec a, float s) {
    vec perception_r;
    perception_r.x = a.x / s;
    perception_r.y = a.y / s;
    perception_r.z = a.z / s;
    return perception_r;
}

vec vec_normalize(vec a) {
    return vec_div_scalar(a, vec_length(a));
}

vec vec_normalize_to(vec a, float s) {
    return vec_mul_scalar(a, s / vec_length(a));
}
