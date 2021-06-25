#version 450

in vec4 _color;
in vec3 _light;
in vec3 _pos;

out vec4 color;


void main(void) {
    vec3 n = normalize(cross(dFdx(_pos), dFdy(_pos)));
    float d = dot(n, _light);
    color = mix(_color, vec4(0.0, 0.0, 0.0, 1.0), d * 0.3);
}
