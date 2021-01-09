#include "shaders.h"


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
\n \
out vec4 _color;\n \
out vec3 _light;\n \
out vec3 _pos;\n \
\n \
uniform mat4 projection;\n \
\n \
\n \
vec4 rotation_between_vectors(vec3 v1, vec3 v2) {\n \
    v1 = normalize(v1);\n \
    v2 = normalize(v2);\n \
\n \
    float cos_theta = dot(v1, v2);\n \
    float s = sqrt((1.0 + cos_theta) * 2.0);\n \
    float invs = 1.0 / s;\n \
\n \
    vec3 axis = cross(v1, v2);\n \
\n \
    return vec4(\n \
        axis.x * invs,\n \
        axis.y * invs,\n \
        axis.z * invs,\n \
        s * 0.5\n \
    );\n \
}\n \
\n \
vec4 quat_multiply(vec4 q1, vec4 q2) {\n \
    return vec4(\n \
        (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y),\n \
        (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x),\n \
        (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w),\n \
        (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z)\n \
    );\n \
}\n \
\n \
void main(void) {\n \
    // vec3 inst_up = vec3(0.0, 0.0, 1.0);\n \
\n \
    vec4 rot1 = rotation_between_vectors(vec3(0.0f, 0.0f, 1.0f), inst_dir);\n \
    // vec3 right = cross(inst_dir, inst_up);\n \
    // inst_up = cross(right, inst_dir);\n \
    // vec3 new_up = rot1.xyz * vec3(0.0f, 1.0f, 0.0f);\n \
    // vec4 rot2 = rotation_between_vectors(new_up, inst_up);\n \
    // vec4 target_orientation = rot2 * rot1;\n \
\n \
    vec4 rotated_pos = quat_multiply(rot1, vec4(pos, 0.0));\n \
\n \
    vec3 actual_pos = rotated_pos.xyz + inst_pos;\n \
    gl_Position = projection * vec4(actual_pos, 1.0);\n \
\n \
    _color = vec4(0.8, 0.8, 0.8, 1.0);\n \
    _light = normalize(actual_pos - vec3(1000, -1000, 1000));\n \
    _pos = actual_pos;\n \
}\n \
";