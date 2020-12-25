

def read(path):
    with open(path, "r") as f:
        return f.read()

_AGENT_H = read('preproc/agent.h')
_AGENT_C = read('preproc/agent.c')
_BUILTINS_C = read('preproc/builtins.c')

# generate opencl kernel source

_OPENCL_SOURCE = """
{0}
{1}
{2}
""".format(
    _AGENT_H,
    _BUILTINS_C,
    _AGENT_C
).replace('__PACK__', '__attribute__((packed))').replace('__GLOBAL__', 'global').replace('\n', '\\n\\\n')

# generate files

with open('agent.h', 'w+') as f:
    f.write("""#ifndef tay_agent_h
#define tay_agent_h

#include "state.h"


#pragma pack(push, 1)

{0}
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
    _AGENT_H
    ).replace('__PACK__ ', '')
)

with open('agent.c', 'w+') as f:
    f.write("""#include "agent.h"


{0}
{1}
const char *agent_kernels_source = "{2}";
""".format(
    _AGENT_C.replace('__GLOBAL__ ', ''),
    _BUILTINS_C,
    _OPENCL_SOURCE
    )
)
