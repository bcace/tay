import sys


builtins_h = """
float2 float2_get_agent_position(void *agent);
float3 float3_get_agent_position(void *agent);
float4 float4_get_agent_position(void *agent);

void float2_set_agent_position(void *agent, float2 p);
void float3_set_agent_position(void *agent, float3 p);
void float4_set_agent_position(void *agent, float4 p);

float3 float3_null();
float3 float3_make(float x, float y, float z);
float3 float3_add(float3 a, float3 b);
float3 float3_sub(float3 a, float3 b);
float3 float3_div_scalar(float3 a, float s);
"""

builtins_c = """
float2 float2_get_agent_position(__GLOBAL__ void *agent) {
    return *(__GLOBAL__ float2 *)((__GLOBAL__ TayAgentTag *)agent + 1);
}

float3 float3_get_agent_position(__GLOBAL__ void *agent) {
    return *(__GLOBAL__ float3 *)((__GLOBAL__ TayAgentTag *)agent + 1);
}

float4 float4_get_agent_position(__GLOBAL__ void *agent) {
    return *(__GLOBAL__ float4 *)((__GLOBAL__ TayAgentTag *)agent + 1);
}

void float2_set_agent_position(__GLOBAL__ void *agent, float2 p) {
    *(__GLOBAL__ float2 *)((__GLOBAL__ TayAgentTag *)agent + 1) = p;
}

void float3_set_agent_position(__GLOBAL__ void *agent, float3 p) {
    *(__GLOBAL__ float3 *)((__GLOBAL__ TayAgentTag *)agent + 1) = p;
}

void float4_set_agent_position(__GLOBAL__ void *agent, float4 p) {
    *(__GLOBAL__ float4 *)((__GLOBAL__ TayAgentTag *)agent + 1) = p;
}

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
