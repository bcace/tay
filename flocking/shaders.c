#include "shaders.h"


const char *flat_vert = "#version 130\n \
\n \
in vec3 pos;\n \
in vec4 color;\n \
\n \
out vec4 _color;\n \
\n \
uniform mat4 projection;\n \
uniform mat4 modelview;\n \
\n \
\n \
void main(void) {\n \
    vec4 modelview_pos = modelview * vec4(pos, 1.0);\n \
    gl_Position = projection * modelview_pos;\n \
\n \
    _color = color;\n \
}\n \
";

const char *flat_frag = "#version 130\n \
\n \
in vec4 _color;\n \
\n \
out vec4 color;\n \
\n \
\n \
void main(void) {\n \
    color = _color;\n \
}\n \
";

const char *shaded_vert = "#version 130\n \
\n \
in vec3 pos;\n \
\n \
out vec4 _color;\n \
out vec3 _light;\n \
out vec3 _pos;\n \
\n \
uniform mat4 projection;\n \
uniform mat4 modelview;\n \
uniform vec4 color;\n \
\n \
\n \
void main(void) {\n \
    vec4 modelview_pos = modelview * vec4(pos, 1.0);\n \
    gl_Position = projection * modelview_pos;\n \
\n \
    vec3 _modelview_pos;\n \
    _modelview_pos.x = modelview_pos.x;\n \
    _modelview_pos.y = modelview_pos.y;\n \
    _modelview_pos.z = modelview_pos.z;\n \
\n \
    _color = color;\n \
    _light = normalize(_modelview_pos - vec3(1000, -1000, 1000));\n \
    _pos = _modelview_pos;\n \
}\n \
";

const char *shaded_frag = "#version 130\n \
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
