#version 130

in vec3 pos;
in vec3 inst_pos;

out vec4 _color;
out vec3 _light;
out vec3 _pos;

uniform mat4 projection;
uniform mat4 modelview;


void main(void) {
    vec3 actual_pos = pos + inst_pos;
    vec4 modelview_pos = modelview * vec4(actual_pos, 1.0);
    gl_Position = projection * modelview_pos;

    vec3 _modelview_pos;
    _modelview_pos.x = modelview_pos.x;
    _modelview_pos.y = modelview_pos.y;
    _modelview_pos.z = modelview_pos.z;

    _color = vec4(1.0, 0.0, 0.0, 1.0);
    _light = normalize(_modelview_pos - vec3(1000, -1000, 1000));
    _pos = _modelview_pos;
}
