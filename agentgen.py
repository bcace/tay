import sys


builtins_h = """
float3 float3_null();
float3 float3_make(float x, float y, float z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_div_scalar(float3 a, float s);
float3 float3_mul_scalar(float3 a, float s);
float3 float3_normalize(float3 a);
float3 float3_normalize_t(float3 a, float b);
float float3_length(float3 a);

float4 float4_null();
float4 float4_make(float x, float y, float z, float w);
float4 float4_add(float4 a, float4 b);
float4 float4_sub(float4 a, float4 b);
float4 float4_div_scalar(float4 a, float s);
"""

builtins_c = """
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
        r.x = b;
        r.y = 0.0f;
        r.z = 0.0f;
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
"""

def _read(path):
    with open(path, "r") as f:
        return f.read()

def main(proj_dir):
    agent_h = _read('%s/preproc/agent.h' % proj_dir)
    agent_c = _read('%s/preproc/agent.c' % proj_dir)

    #
    # OpenCL source string
    #

    opencl_string = """
{0}
{1}
{2}
""".format(
    agent_h,
    builtins_c,
    agent_c
).replace('__PACK__', '__attribute__((packed))').replace('__GLOBAL__', 'global').replace('\n', '\\n\\\n')

    #
    # Write header file
    #

    with open('%s/agent.h' % proj_dir, 'w+') as f:
        f.write("""#ifndef tay_agent_h
#define tay_agent_h

#include "tay.h"


#pragma pack(push, 1)

{0}
void agent_see(Agent *a, Agent *b, SeeContext *context);
void agent_act(Agent *agent, ActContext *context);

{1}
extern const char *agent_kernels_source;

#pragma pack(pop)

#endif
""".format(
    agent_h,
    builtins_h
    ).replace('__PACK__ ', '')
)

    #
    # Write source file
    #

    with open('%s/agent.c' % proj_dir, 'w+') as f:
        f.write("""#include "agent.h"
#include "tay.h"
#include <math.h>


{0}
{1}
const char *agent_kernels_source = "{2}";
""".format(
    agent_c.replace('__GLOBAL__ ', ''),
    builtins_c.replace('__GLOBAL__ ', ''),
    opencl_string
    )
)


if __name__ == "__main__":
   main(sys.argv[1])
