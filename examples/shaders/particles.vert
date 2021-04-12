#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 inst_pos;
layout(location = 2) in float inst_size;

out vec4 _color;
out vec3 _light;
out vec3 _pos;

uniform mat4 projection;


void main(void) {
    vec3 actual_pos = pos * inst_size + inst_pos;
    gl_Position = projection * vec4(actual_pos, 1.0);

    _color = vec4(0.4, 0.5, 0.6, 1.0);
    _light = normalize(actual_pos - vec3(1000, -1000, 1000));
    _pos = actual_pos;
}
