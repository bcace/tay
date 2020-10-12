

def read(path):
    with open(path, "r") as f:
        return f.read()


agent_h = read('gen/agent.h')
agent_c = read('gen/agent.c')
builtins_h = read('gen/builtins.h')
builtins_c = read('gen/builtins.c')

# generate opencl kernel source

kernels = """
kernel void see_kernel(global Agent *agents, global SeeContext *see_context) {
}

kernel void act_kernel(global Agent *agents, global ActContext *act_context) {
}"""

kernels_cl = """
{0}
{1}
{2}
{3}
""".format(
    agent_h,
    builtins_c,
    agent_c,
    kernels
).replace('__PACK__', '__attribute__((packed))').replace('__GLOBAL__', 'global').replace("\n", "\\n \\\n")

# generate files

with open('agent.h', 'w+') as f:
    f.write("""#ifndef agent_h
#define agent_h

#pragma pack(push, 1)

{0}
{1}
void agent_see(Agent *a, Agent *b, SeeContext *context);
void agent_act(Agent *agent, ActContext *context);

float4 float4_null();
float4 float4_make(float x, float y, float z);
float4 float4_add(float4 a, float4 b);
float4 float4_sub(float4 a, float4 b);
float4 float4_div_scalar(float4 a, float s);

extern const char *agent_kernels_source;

#pragma pack(pop)

#endif
""".format(
    builtins_h,
    agent_h.replace('__PACK__ ', '')
    )
)

with open('agent.c', 'w+') as f:
    f.write("""#include "agent.h"


{0}
{1}
const char *agent_kernels_source = "{2}";
""".format(
    agent_c.replace('__GLOBAL__ ', ''),
    builtins_c,
    kernels_cl
    )
)
