#include "ocl.h"
#include "state.h"
#include <stdio.h>

#define _MAX_GENERICS_TEXT_SIZE 1024


const char *_NARROW_POINT_POINT[];
const char *_NARROW_POINT_NONPOINT[];
const char *_NARROW_NONPOINT_POINT[];
const char *_NARROW_NONPOINT_NONPOINT[];

const char *ocl_get_seer_agent_text(TayPass *pass) {
    #ifdef TAY_OCL

    if (pass->seer_group->is_point)
        return "\
    global void *a = a_agents + a_size * a_i;\n\
    float4 a_p = float4_agent_position(a);\n\
    float4 box_min = a_p - radii;\n\
    float4 box_max = a_p + radii;\n";
    else
        return "\
    global void *a = a_agents + a_size * a_i;\n\
    float4 a_min = float4_agent_min(a);\n\
    float4 a_max = float4_agent_max(a);\n\
    float4 box_min = a_min - radii;\n\
    float4 box_max = a_max + radii;\n";

    #else

    return "";

    #endif
}

char *ocl_get_coupling_text(TayPass *pass, int dims) {
    static char text[_MAX_GENERICS_TEXT_SIZE];
    text[0] = '\0';

    #ifdef TAY_OCL

    const char *seen_position;
    if (pass->seen_group->is_point)
        seen_position = "\
    float4 b_p = float4_agent_position(b);\n";
    else
        seen_position = "\
    float4 b_min = float4_agent_min(b);\n\
    float4 b_max = float4_agent_max(b);\n";

    const char *self_see;
    if (pass->seer_group == pass->seen_group && !pass->self_see)
        self_see = "\n\
    if (a_i == b_i)\n\
        goto SKIP_SEE;\n";
    else
        self_see = "";

    const char *narrow_phase = 0;
    if (pass->seer_group->is_point && pass->seen_group->is_point)
        narrow_phase = _NARROW_POINT_POINT[dims - 1];
    else if (pass->seer_group->is_point)
        narrow_phase = _NARROW_POINT_NONPOINT[dims - 1];
    else if (pass->seen_group->is_point)
        narrow_phase = _NARROW_NONPOINT_POINT[dims - 1];
    else
        narrow_phase = _NARROW_NONPOINT_NONPOINT[dims - 1];

    unsigned length = sprintf_s(text, _MAX_GENERICS_TEXT_SIZE, "\
/* pairing start */\n\
for (unsigned b_i = b_beg; b_i < b_end; ++b_i) {\n\
    global void *b = b_agents + b_size * b_i;\n\
%s\
%s\
%s\
    %s(a, b, c);\n\
\n\
    SKIP_SEE:;\n\
}\n\
/* pairing end */\n",
    seen_position, self_see, narrow_phase, pass->func_name);

    #endif

    return text;
}

const char *_NARROW_POINT_POINT[] = {
"\n\
    float dx = a_p.x - b_p.x;\n\
    if (dx < -radii.x || dx > radii.x)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float dx = a_p.x - b_p.x;\n\
    if (dx < -radii.x || dx > radii.x)\n\
        goto SKIP_SEE;\n\
    float dy = a_p.y - b_p.y;\n\
    if (dy < -radii.y || dy > radii.y)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float dx = a_p.x - b_p.x;\n\
    if (dx < -radii.x || dx > radii.x)\n\
        goto SKIP_SEE;\n\
    float dy = a_p.y - b_p.y;\n\
    if (dy < -radii.y || dy > radii.y)\n\
        goto SKIP_SEE;\n\
    float dz = a_p.z - b_p.z;\n\
    if (dz < -radii.z || dz > radii.z)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float dx = a_p.x - b_p.x;\n\
    if (dx < -radii.x || dx > radii.x)\n\
        goto SKIP_SEE;\n\
    float dy = a_p.y - b_p.y;\n\
    if (dy < -radii.y || dy > radii.y)\n\
        goto SKIP_SEE;\n\
    float dz = a_p.z - b_p.z;\n\
    if (dz < -radii.z || dz > radii.z)\n\
        goto SKIP_SEE;\n\
    float dw = a_p.w - b_p.w;\n\
    if (dw < -radii.w || dw > radii.w)\n\
        goto SKIP_SEE;\n\
\n"
};

const char *_NARROW_POINT_NONPOINT[] = {
"\n\
    float d;\n\
    d = a_p.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = a_p.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_p.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = a_p.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_p.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_p.z - b_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_min.z - a_p.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = a_p.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_p.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_p.z - b_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_min.z - a_p.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = a_p.w - b_max.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
    d = b_min.w - a_p.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
\n"
};

const char *_NARROW_NONPOINT_POINT[] = {
"\n\
    float d;\n\
    d = b_p.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.x - b_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = b_p.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.x - b_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_p.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = b_p.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.x - b_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_p.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_p.z - a_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = a_min.z - b_p.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = b_p.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.x - b_p.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_p.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_p.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_p.z - a_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = a_min.z - b_p.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_p.w - a_max.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
    d = a_min.w - b_p.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
\n"
};

const char *_NARROW_NONPOINT_NONPOINT[] =
{"\n\
    float d;\n\
    d = a_min.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = a_min.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = a_min.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.z - b_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_min.z - a_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
\n",
"\n\
    float d;\n\
    d = a_min.x - b_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = b_min.x - a_max.x;\n\
    if (d > radii.x)\n\
        goto SKIP_SEE;\n\
    d = a_min.y - b_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = b_min.y - a_max.y;\n\
    if (d > radii.y)\n\
        goto SKIP_SEE;\n\
    d = a_min.z - b_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = b_min.z - a_max.z;\n\
    if (d > radii.z)\n\
        goto SKIP_SEE;\n\
    d = a_min.w - b_max.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
    d = b_min.w - a_max.w;\n\
    if (d > radii.w)\n\
        goto SKIP_SEE;\n\
\n"
};
