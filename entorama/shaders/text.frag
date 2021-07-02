#version 450

in vec4 _color;
in vec2 _tex_pos;

out vec4 color;

uniform sampler2D tex;


void main(void) {
    vec4 sss = texture(tex, _tex_pos);
    color = _color;
    color.a = sss.a;
}
