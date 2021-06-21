
float3 float3_null();
float3 float3_make(float x, float y, float z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_div_scalar(float3 a, float s);
float3 float3_mul_scalar(float3 a, float s);
float3 float3_normalize(float3 a);
float3 float3_normalize_to(float3 a, float b);
float float3_length(float3 a);
float float3_dot(float3 a, float3 b);

float4 float4_null();
float4 float4_make(float x, float y, float z, float w);
float4 float4_add(float4 a, float4 b);
float4 float4_sub(float4 a, float4 b);
float4 float4_div_scalar(float4 a, float s);
float4 float4_mul_scalar(float4 a, float s);
float4 float4_normalize(float4 a);
float4 float4_normalize_to(float4 a, float b);
float float4_length(float4 a);
float float4_dot(float4 a, float4 b);

float4 float2x2_make(float v);
float4 float2x2_add_scalar(float4 m, float s);
float4 float2x2_multiply_scalar(float4 m, float s);
float2 float2x2_multiply_vector(float4 m, float2 v);
float4 float2x2_add(float4 a, float4 b);
float4 float2x2_subtract(float4 a, float4 b);
float4 float2x2_multiply(float4 a, float4 b);
float float2x2_determinant(float4 m);
float4 float2x2_transpose(float4 m);

float2 float2_null();
float2 float2_add(float2 a, float2 b);
float2 float2_mul_scalar(float2 a, float s);
float4 float2_outer_product(float2 a, float2 b);
