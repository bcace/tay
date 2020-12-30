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
layout(location = 0) in vec4 pos;\n \
layout(location = 1) in vec4 inst_pos;\n \
layout(location = 2) in vec4 inst_dir;\n \
\n \
out vec4 _color;\n \
out vec3 _light;\n \
out vec3 _pos;\n \
\n \
uniform mat4 projection;\n \
uniform mat4 modelview;\n \
\n \
\n \
void main(void) {\n \
    float hhh = inst_dir.x;\n \
\n \
    vec3 actual_pos = pos.xyz + inst_pos.xyz;\n \
    vec4 modelview_pos = modelview * vec4(actual_pos, 1.0);\n \
    gl_Position = projection * modelview_pos;\n \
\n \
    vec3 _modelview_pos;\n \
    _modelview_pos.x = modelview_pos.x;\n \
    _modelview_pos.y = modelview_pos.y;\n \
    _modelview_pos.z = modelview_pos.z;\n \
\n \
    _color = vec4(1.0, 0.0, 0.0, 1.0);\n \
    _light = normalize(_modelview_pos - vec3(1000, -1000, 1000));\n \
    _pos = _modelview_pos;\n \
}\n \
";
