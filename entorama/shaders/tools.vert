#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

uniform mat4 projection;
uniform mat4 modelview;

out vec4 _color;


void main(void) {
    vec4 global_pos = modelview * vec4(pos, 1.0);
    gl_Position = projection * global_pos;

    _color = color;
}
