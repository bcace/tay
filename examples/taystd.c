
float3 float3_null() {
    float3 r;
    r.x = 0.0f;
    r.y = 0.0f;
    r.z = 0.0f;
    return r;
}

float3 float3_make(float x, float y, float z) {
    float3 r;
    r.x = x;
    r.y = y;
    r.z = z;
    return r;
}

float3 float3_add(float3 a, float3 b) {
    float3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

float3 float3_sub(float3 a, float3 b) {
    float3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

float3 float3_div_scalar(float3 a, float s) {
    float3 r;
    r.x = a.x / s;
    r.y = a.y / s;
    r.z = a.z / s;
    return r;
}

float3 float3_mul_scalar(float3 a, float s) {
    float3 r;
    r.x = a.x * s;
    r.y = a.y * s;
    r.z = a.z * s;
    return r;
}

float3 float3_normalize(float3 a) {
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    float3 r;
    if (l < 0.000001f) {
        r.x = 1.0f;
        r.y = 0.0f;
        r.z = 0.0f;
    }
    else {
        r.x = a.x / l;
        r.y = a.y / l;
        r.z = a.z / l;
    }
    return r;
}

float3 float3_normalize_to(float3 a, float b) {
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    float3 r;
    if (l < 0.000001f) {
        r.x = a.x;
        r.y = a.y;
        r.z = a.z;
    }
    else {
        float c = b / l;
        r.x = a.x * c;
        r.y = a.y * c;
        r.z = a.z * c;
    }
    return r;
}

float float3_length(float3 a) {
    return (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

float float3_dot(float3 a, float3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float4 float4_null() {
    float4 r;
    r.x = 0.0f;
    r.y = 0.0f;
    r.z = 0.0f;
    r.w = 0.0f;
    return r;
}

float4 float4_make(float x, float y, float z, float w) {
    float4 r;
    r.x = x;
    r.y = y;
    r.z = z;
    r.w = w;
    return r;
}

float4 float4_add(float4 a, float4 b) {
    float4 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    r.w = a.w + b.w;
    return r;
}

float4 float4_sub(float4 a, float4 b) {
    float4 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    r.w = a.w - b.w;
    return r;
}

float4 float4_div_scalar(float4 a, float s) {
    float4 r;
    r.x = a.x / s;
    r.y = a.y / s;
    r.z = a.z / s;
    r.w = a.w / s;
    return r;
}


float4 float4_mul_scalar(float4 a, float s) {
    float4 r;
    r.x = a.x * s;
    r.y = a.y * s;
    r.z = a.z * s;
    return r;
}

float4 float4_normalize(float4 a) {
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    float4 r;
    if (l < 0.000001f) {
        r.x = 1.0f;
        r.y = 0.0f;
        r.z = 0.0f;
    }
    else {
        r.x = a.x / l;
        r.y = a.y / l;
        r.z = a.z / l;
    }
    return r;
}

float4 float4_normalize_to(float4 a, float b) {
    float l = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    float4 r;
    if (l < 0.000001f) {
        r.x = a.x;
        r.y = a.y;
        r.z = a.z;
    }
    else {
        float c = b / l;
        r.x = a.x * c;
        r.y = a.y * c;
        r.z = a.z * c;
    }
    return r;
}

float float4_length(float4 a) {
    return (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

float float4_dot(float4 a, float4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float4 float2x2_make(float v) {
    return (float4) {
        v,
        0.0f,
        0.0f,
        v,
    };
}

float4 float2x2_add_scalar(float4 m, float s) {
    return (float4) {
        m.x + s,
        m.y + s,
        m.z + s,
        m.w + s,
    };
}

float4 float2x2_multiply_scalar(float4 m, float s) {
    return (float4) {
        m.x * s,
        m.y * s,
        m.z * s,
        m.w * s,
    };
}

float2 float2x2_multiply_vector(float4 m, float2 v) {
    return (float2) {
        m.x * v.x + m.y * v.y,
        m.z * v.x + m.w * v.y,
    };
}

float4 float2x2_add(float4 a, float4 b) {
    return (float4) {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w,
    };
}

float4 float2x2_subtract(float4 a, float4 b) {
    return (float4) {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w,
    };
}

float float2x2_determinant(float4 m) {
    return m.x * m.w - m.y * m.z;
}

float4 float2x2_transpose(float4 m) {
    return (float4) {
        m.x,
        m.z,
        m.y,
        m.w,
    };
}

float4 float2x2_multiply(float4 a, float4 b) {
    return (float4) {
        a.x * b.x + a.y * b.z,
        a.x * b.y + a.y * b.w,
        a.z * b.x + a.w * b.z,
        a.z * b.y + a.w * b.w,
    };
}

float2 float2_null() {
    return (float2) { 0.0f, 0.0f };
}
