#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex_pos;

uniform mat4 projection;
uniform vec4 uniform_color;

out vec4 _color;
out vec2 _tex_pos;


void main(void) {
    gl_Position = projection * vec4(vec3(pos, 0.0f), 1.0f);

    _color = uniform_color;
    _tex_pos = tex_pos;
}
