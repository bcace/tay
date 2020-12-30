#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 inst_pos;
layout(location = 2) in vec3 inst_dir;

out vec4 _color;
out vec3 _light;
out vec3 _pos;

uniform mat4 projection;
uniform mat4 modelview;


vec4 rotation_between_vectors(vec3 v1, vec3 v2) {
    v1 = normalize(v1);
    v2 = normalize(v2);

    float cos_theta = dot(v1, v2);
    float theta_2 = acos(cos_theta) * 0.5;
    float sin_theta_2 = sin(theta_2);

    vec3 axis = cross(v1, v2);

    vec4 q = vec4(
        axis.x * sin_theta_2,
        axis.y * sin_theta_2,
        axis.z * sin_theta_2,
        cos(theta_2)
    );

    // return normalize(q);
    return q;
}

vec4 quat_multiply(vec4 q1, vec4 q2) {
    return vec4(
        (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y),
        (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x),
        (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w),
        (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z)
    );
}

void main(void) {
    // vec3 inst_up = vec3(0.0, 0.0, 1.0);

    vec4 rot1 = rotation_between_vectors(vec3(0.0f, 0.0f, 1.0f), inst_dir);
    // vec3 right = cross(inst_dir, inst_up);
    // inst_up = cross(right, inst_dir);
    // vec3 new_up = rot1.xyz * vec3(0.0f, 1.0f, 0.0f);
    // vec4 rot2 = rotation_between_vectors(new_up, inst_up);
    // vec4 target_orientation = rot2 * rot1;

    vec4 rotated_pos = quat_multiply(rot1, vec4(pos, 0.0));

    vec3 actual_pos = rotated_pos.xyz + inst_pos;
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
