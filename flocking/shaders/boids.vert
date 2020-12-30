#version 450

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 inst_pos;
layout(location = 2) in vec4 inst_dir;

out vec4 _color;
out vec3 _light;
out vec3 _pos;

uniform mat4 projection;
uniform mat4 modelview;


void main(void) {
    float hhh = inst_dir.x;

    vec3 actual_pos = pos.xyz + inst_pos.xyz;
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
