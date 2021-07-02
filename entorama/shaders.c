#include "shaders.h"


const char *agent_frag = "\n \
in vec4 _color;\n \
in vec3 _light;\n \
in vec3 _pos;\n \
\n \
out vec4 color;\n \
\n \
\n \
void main(void) {\n \
    vec3 n = normalize(cross(dFdx(_pos), dFdy(_pos)));\n \
    float d = dot(n, _light);\n \
    color = mix(_color, vec4(0.0, 0.0, 0.0, 1.0), d * 0.3);\n \
}\n \
";

const char *agent_vert = "\n \
layout(location = 0) in vec3 pos;\n \
layout(location = 1) in vec3 inst_pos;\n \
layout(location = 2) in vec3 inst_dir_fwd;\n \
layout(location = 3) in vec4 inst_color;\n \
layout(location = 4) in vec3 inst_size;\n \
\n \
out vec4 _color;\n \
out vec3 _light;\n \
out vec3 _pos;\n \
\n \
uniform mat4 projection;\n \
uniform mat4 modelview;\n \
uniform vec4 uniform_color;\n \
uniform vec3 uniform_size;\n \
\n \
\n \
vec4 _rotation_between_vectors(vec3 v1, vec3 v2) {\n \
    v1 = normalize(v1);\n \
    v2 = normalize(v2);\n \
\n \
    vec3 u = cross(v1, v2);\n \
    u = normalize(u);\n \
\n \
    float theta = acos(dot(v1, v2)) * 0.5f;\n \
    float c = cos(theta);\n \
    float s = sin(theta);\n \
\n \
    return vec4(\n \
        u.x * s,\n \
        u.y * s,\n \
        u.z * s,\n \
        c\n \
    );\n \
}\n \
\n \
vec3 _quat_rotate(vec4 q, vec3 v) {\n \
    vec3 r;\n \
\n \
    float ww = q.w * q.w;\n \
    float xx = q.x * q.x;\n \
    float yy = q.y * q.y;\n \
    float zz = q.z * q.z;\n \
    float wx = q.w * q.x;\n \
    float wy = q.w * q.y;\n \
    float wz = q.w * q.z;\n \
    float xy = q.x * q.y;\n \
    float xz = q.x * q.z;\n \
    float yz = q.y * q.z;\n \
\n \
    r.x = ww * v.x + 2.0f * wy * v.z - 2.0f * wz * v.y +\n \
          xx * v.x + 2.0f * xy * v.y + 2.0f * xz * v.z -\n \
          zz * v.x - yy * v.x;\n \
    r.y = 2.0f * xy * v.x + yy * v.y + 2.0f * yz * v.z +\n \
          2.0f * wz * v.x - zz * v.y + ww * v.y -\n \
          2.0f * wx * v.z - xx * v.y;\n \
    r.z = 2.0f * xz * v.x + 2.0f * yz * v.y + zz * v.z -\n \
          2.0f * wy * v.x - yy * v.z + 2.0f * wx * v.y -\n \
          xx * v.z + ww * v.z;\n \
\n \
    return r;\n \
}\n \
\n \
void main(void) {\n \
    vec3 local_pos = pos;\n \
\n \
#ifdef ENTORAMA_DIRECTION_FWD\n \
    vec4 rot1 = _rotation_between_vectors(vec3(0.0f, 0.0f, 1.0f), inst_dir_fwd);\n \
    local_pos = _quat_rotate(rot1, local_pos);\n \
#endif\n \
\n \
#ifdef ENTORAMA_SIZE_UNIFORM\n \
    local_pos *= uniform_size;\n \
#endif\n \
\n \
#ifdef ENTORAMA_SIZE_AGENT\n \
    local_pos *= inst_size;\n \
#endif\n \
\n \
    vec4 global_pos = modelview * vec4(local_pos + inst_pos, 1.0);\n \
\n \
    gl_Position = projection * global_pos;\n \
\n \
#ifdef ENTORAMA_COLOR_AGENT\n \
    _color = inst_color;\n \
#else\n \
    _color = uniform_color;\n \
#endif\n \
\n \
    _light = normalize(global_pos.xyz - vec3(1000, -1000, 1000));\n \
    _pos = global_pos.xyz;\n \
}\n \
";

const char *boids_frag = "#version 450\n \
\n \
in vec4 _color;\n \
in vec3 _light;\n \
in vec3 _pos;\n \
\n \
out vec4 color;\n \
\n \
\n \
void main(void) {\n \
    vec3 n = normalize(cross(dFdx(_pos), dFdy(_pos)));\n \
    float d = dot(n, _light);\n \
    color = mix(_color, vec4(0.0, 0.0, 0.0, 1.0), d * 0.3);\n \
}\n \
";

const char *boids_vert = "#version 450\n \
\n \
layout(location = 0) in vec3 pos;\n \
layout(location = 1) in vec3 inst_pos;\n \
layout(location = 2) in vec3 inst_dir;\n \
layout(location = 3) in float inst_shd;\n \
\n \
out vec4 _color;\n \
out vec3 _light;\n \
out vec3 _pos;\n \
\n \
uniform mat4 projection;\n \
uniform mat4 modelview;\n \
\n \
\n \
vec4 rotation_between_vectors(vec3 v1, vec3 v2) {\n \
    v1 = normalize(v1);\n \
    v2 = normalize(v2);\n \
\n \
    vec3 u = cross(v1, v2);\n \
    u = normalize(u);\n \
\n \
    float theta = acos(dot(v1, v2)) * 0.5f;\n \
    float c = cos(theta);\n \
    float s = sin(theta);\n \
\n \
    return vec4(\n \
        u.x * s,\n \
        u.y * s,\n \
        u.z * s,\n \
        c\n \
    );\n \
}\n \
\n \
vec4 quat_multiply(vec4 q1, vec4 q2) {\n \
    return vec4(\n \
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,\n \
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,\n \
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,\n \
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z\n \
    );\n \
}\n \
\n \
vec3 quat_rotate(vec4 q, vec3 v) {\n \
    vec3 r;\n \
\n \
    float ww = q.w * q.w;\n \
    float xx = q.x * q.x;\n \
    float yy = q.y * q.y;\n \
    float zz = q.z * q.z;\n \
    float wx = q.w * q.x;\n \
    float wy = q.w * q.y;\n \
    float wz = q.w * q.z;\n \
    float xy = q.x * q.y;\n \
    float xz = q.x * q.z;\n \
    float yz = q.y * q.z;\n \
\n \
    r.x = ww * v.x + 2.0f * wy * v.z - 2.0f * wz * v.y +\n \
          xx * v.x + 2.0f * xy * v.y + 2.0f * xz * v.z -\n \
          zz * v.x - yy * v.x;\n \
    r.y = 2.0f * xy * v.x + yy * v.y + 2.0f * yz * v.z +\n \
          2.0f * wz * v.x - zz * v.y + ww * v.y -\n \
          2.0f * wx * v.z - xx * v.y;\n \
    r.z = 2.0f * xz * v.x + 2.0f * yz * v.y + zz * v.z -\n \
          2.0f * wy * v.x - yy * v.z + 2.0f * wx * v.y -\n \
          xx * v.z + ww * v.z;\n \
\n \
    return r;\n \
}\n \
\n \
void main(void) {\n \
    vec4 rot1 = rotation_between_vectors(vec3(0.0f, 0.0f, 1.0f), inst_dir);\n \
    vec3 rot_pos = quat_rotate(rot1, pos);\n \
\n \
    vec4 actual_pos = modelview * vec4(rot_pos + inst_pos, 1.0);\n \
    gl_Position = projection * actual_pos;\n \
\n \
    if (inst_shd == 0.0)\n \
        _color = vec4(0.4, 0.4, 0.4, 1.0);\n \
    else\n \
        _color = vec4(1.0, 0.0, 0.0, 1.0);\n \
    _light = normalize(actual_pos.xyz - vec3(1000, -1000, 1000));\n \
    _pos = actual_pos.xyz;\n \
}\n \
";

const char *particles_frag = "#version 450\n \
\n \
in vec4 _color;\n \
in vec3 _light;\n \
in vec3 _pos;\n \
\n \
out vec4 color;\n \
\n \
\n \
void main(void) {\n \
    vec3 n = normalize(cross(dFdx(_pos), dFdy(_pos)));\n \
    float d = dot(n, _light);\n \
    color = mix(_color, vec4(0.0, 0.0, 0.0, 1.0), d * 0.3);\n \
}\n \
";

const char *particles_vert = "#version 450\n \
\n \
layout(location = 0) in vec3 pos;\n \
layout(location = 1) in vec3 inst_pos;\n \
layout(location = 2) in float inst_size;\n \
layout(location = 3) in float inst_energy;\n \
\n \
out vec4 _color;\n \
out vec3 _light;\n \
out vec3 _pos;\n \
\n \
uniform mat4 projection;\n \
\n \
\n \
void main(void) {\n \
    vec3 actual_pos = pos * inst_size + inst_pos;\n \
    gl_Position = projection * vec4(actual_pos, 1.0);\n \
\n \
    _color = mix(vec4(0.0, 0.3, 0.4, 1.0), vec4(1.0, 1.0, 1.0, 1.0), inst_energy);\n \
    _light = normalize(actual_pos - vec3(1000, -1000, 1000));\n \
    _pos = actual_pos;\n \
}\n \
";

const char *text_frag = "#version 450\n \
\n \
in vec4 _color;\n \
in vec2 _tex_pos;\n \
\n \
out vec4 color;\n \
\n \
uniform sampler2D tex;\n \
\n \
\n \
void main(void) {\n \
    vec4 sss = texture(tex, _tex_pos);\n \
    color = _color;\n \
    color.a = sss.a;\n \
}\n \
";

const char *text_vert = "#version 450\n \
\n \
layout(location = 0) in vec2 pos;\n \
layout(location = 1) in vec2 tex_pos;\n \
\n \
uniform mat4 projection;\n \
uniform vec4 uniform_color;\n \
\n \
out vec4 _color;\n \
out vec2 _tex_pos;\n \
\n \
\n \
void main(void) {\n \
    gl_Position = projection * vec4(vec3(pos, 0.0f), 1.0f);\n \
\n \
    _color = uniform_color;\n \
    _tex_pos = tex_pos;\n \
}\n \
";
