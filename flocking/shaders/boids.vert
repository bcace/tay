#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 inst_pos;
layout(location = 2) in vec3 inst_dir;

out vec4 _color;
out vec3 _light;
out vec3 _pos;

uniform mat4 projection;


vec4 rotation_between_vectors(vec3 v1, vec3 v2) {
    v1 = normalize(v1);
    v2 = normalize(v2);

    vec3 u = cross(v1, v2);
    u = normalize(u);

    float theta = acos(dot(v1, v2)) * 0.5f;
    float c = cos(theta);
    float s = sin(theta);

    return vec4(
        u.x * s,
        u.y * s,
        u.z * s,
        c
    );
}

vec4 quat_multiply(vec4 q1, vec4 q2) {
    return vec4(
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
    );
}

vec3 quat_rotate(vec4 q, vec3 v) {
    vec3 r;

    float ww = q.w * q.w;
    float xx = q.x * q.x;
    float yy = q.y * q.y;
    float zz = q.z * q.z;
    float wx = q.w * q.x;
    float wy = q.w * q.y;
    float wz = q.w * q.z;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float yz = q.y * q.z;

    r.x = ww * v.x + 2.0f * wy * v.z - 2.0f * wz * v.y +
          xx * v.x + 2.0f * xy * v.y + 2.0f * xz * v.z -
          zz * v.x - yy * v.x;
    r.y = 2.0f * xy * v.x + yy * v.y + 2.0f * yz * v.z +
          2.0f * wz * v.x - zz * v.y + ww * v.y -
          2.0f * wx * v.z - xx * v.y;
    r.z = 2.0f * xz * v.x + 2.0f * yz * v.y + zz * v.z -
          2.0f * wy * v.x - yy * v.z + 2.0f * wx * v.y -
          xx * v.z + ww * v.z;

    return r;
}

void main(void) {
    // vec3 inst_up = vec3(0.0, 0.0, 1.0);

    vec4 rot1 = rotation_between_vectors(vec3(0.0f, 0.0f, 1.0f), inst_dir);
    // vec3 right = cross(inst_dir, inst_up);
    // inst_up = cross(right, inst_dir);
    // vec3 new_up = rot1.xyz * vec3(0.0f, 1.0f, 0.0f);
    // vec4 rot2 = rotation_between_vectors(new_up, inst_up);
    // vec4 target_orientation = rot2 * rot1;

    // vec4 rotated_pos = quat_multiply(rot1, vec4(pos, 0.0));
    vec3 rot_pos = quat_rotate(rot1, pos);

    vec3 actual_pos = rot_pos + inst_pos;
    gl_Position = projection * vec4(actual_pos, 1.0);

    _color = vec4(0.8, 0.8, 0.8, 1.0);
    _light = normalize(actual_pos - vec3(1000, -1000, 1000));
    _pos = actual_pos;
}
