#ifndef tay_vec_h
#define tay_vec_h


typedef struct vec {
    float x, y, z;
} vec;

vec vec_null();
vec vec_make(float x, float y, float z);
vec vec_rand_scalar(float min, float max);
vec vec_rand(vec min, vec max);
vec vec_sub(vec a, vec b);
vec vec_add(vec a, vec b);
float vec_length(vec a);
vec vec_mul_scalar(vec a, float s);
vec vec_div_scalar(vec a, float s);
vec vec_normalize(vec a);
vec vec_normalize_to(vec a, float s);

#endif
