#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 color;

uniform mat4 projection;

out vec4 _color;


void main(void) {
    gl_Position = projection * vec4(vec3(pos, 0.0f), 1.0f);

    _color = color;
}
