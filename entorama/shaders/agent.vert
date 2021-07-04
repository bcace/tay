
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 inst_pos;
layout(location = 2) in vec3 inst_dir_fwd;
layout(location = 3) in vec4 inst_color;
layout(location = 4) in vec3 inst_size;

out vec4 _color;
out vec3 _light;
out vec3 _pos;

uniform mat4 projection;
uniform mat4 modelview;
uniform vec4 uniform_color;
uniform vec3 uniform_size;


vec4 _rotation_between_vectors(vec3 v1, vec3 v2) {
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

vec3 _quat_rotate(vec4 q, vec3 v) {
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
    vec3 local_pos = pos;

#ifdef ENTORAMA_DIRECTION_FWD
    vec4 rot1 = _rotation_between_vectors(vec3(0.0f, 0.0f, 1.0f), inst_dir_fwd);
    local_pos = _quat_rotate(rot1, local_pos);
#endif

#ifdef ENTORAMA_SIZE_AGENT
    local_pos *= inst_size;
#else
    local_pos *= uniform_size;
#endif

    vec4 global_pos = modelview * vec4(local_pos + inst_pos, 1.0);

    gl_Position = projection * global_pos;

#ifdef ENTORAMA_COLOR_AGENT
    _color = inst_color;
#else
    _color = uniform_color;
#endif

    _light = normalize(global_pos.xyz - vec3(1000, -1000, 1000));
    _pos = global_pos.xyz;
}
