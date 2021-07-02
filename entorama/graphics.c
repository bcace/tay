#include "graphics.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>


void _create_shader(GLuint program, const char *text, const char *title, const char *defines, GLenum type) {
    GLuint shader = glCreateShader(type);
    const GLchar *sources[] = {defines, text};
    glShaderSource(shader, 2, sources, 0);
    glCompileShader(shader);
    GLint status;
    GLchar error_message[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        glGetShaderInfoLog(shader, 1024, 0, error_message);
        fprintf(stderr, "Error in %s: %s\n", title, error_message);
    }
    glAttachShader(program, shader);
}

void shader_program_init(Program *p, const char *vert_src, const char *vert_title, const char *vert_defines, const char *frag_src, const char *frag_title, const char *frag_defines) {
    p->vbo_count = 0;
    p->uniforms_count = 0;
    p->prog = glCreateProgram();
    _create_shader(p->prog, vert_src, vert_title, vert_defines, GL_VERTEX_SHADER);
    _create_shader(p->prog, frag_src, frag_title, frag_defines, GL_FRAGMENT_SHADER);
    glLinkProgram(p->prog);

    GLint status;
    GLchar error_message[1024];
    glGetProgramiv(p->prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        glGetProgramInfoLog(p->prog, 1024, 0, error_message);
        fprintf(stderr, "Link error for program with shaders %s and %s: %s\n", vert_title, frag_title, error_message);
    }

    glGenVertexArrays(1, &p->vao);
    glBindVertexArray(p->vao);
}

void shader_program_use(Program *p) {
    glUseProgram(p->prog);
    glBindVertexArray(p->vao);
}

void shader_program_define_uniform(Program *p, const char *name) {
    assert(p->uniforms_count < GRAPH_MAX_UNIFORMS);
    p->uniforms[p->uniforms_count] = glGetUniformLocation(p->prog, name);
    ++p->uniforms_count;
}

void shader_program_set_uniform_mat4(Program *p, int uniform_index, mat4 *mat) {
    assert(uniform_index < p->uniforms_count);
    glUniformMatrix4fv(p->uniforms[uniform_index], 1, GL_FALSE, (float *)mat->v);
}

void shader_program_set_uniform_vec4(Program *p, int uniform_index, vec4 *vec) {
    assert(uniform_index < p->uniforms_count);
    glUniform4f(p->uniforms[uniform_index], vec->x, vec->y, vec->z, vec->w);
}

void shader_program_set_uniform_vec3(Program *p, int uniform_index, vec3 *vec) {
    assert(uniform_index < p->uniforms_count);
    glUniform3f(p->uniforms[uniform_index], vec->x, vec->y, vec->z);
}

void shader_program_define_in_float(Program *p, int components) {
    assert(p->vbo_count < GRAPH_MAX_VBOS);
    int vbo_index = p->vbo_count++; /* this assumes vertex buffer index and attribute index (layout) are the same thing */
    glGenBuffers(1, &p->vbos[vbo_index]);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbos[vbo_index]);
    glVertexAttribPointer(vbo_index, components, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(vbo_index);
}

void shader_program_define_instanced_in_float(Program *p, int components) {
    assert(p->vbo_count < GRAPH_MAX_VBOS);
    int vbo_index = p->vbo_count++; /* this assumes vertex buffer index and attribute index (layout) are the same thing */
    glGenBuffers(1, &p->vbos[vbo_index]);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbos[vbo_index]);
    glEnableVertexAttribArray(vbo_index);
    glVertexAttribPointer(vbo_index, components, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glVertexAttribDivisor(vbo_index, 1);
}

void shader_program_set_data_float(Program *p, int vbo_index, int count, int components, void *data) {
    assert(vbo_index < p->vbo_count);
    glBindBuffer(GL_ARRAY_BUFFER, p->vbos[vbo_index]);
    glBufferData(GL_ARRAY_BUFFER, count * components * sizeof(float), data, GL_STATIC_DRAW);
}

void graphics_enable_depth_test(int enable) {
    if (enable)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void graphics_enable_blend(int enable) {
    if (enable) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
        glDisable(GL_BLEND);
}

void graphics_enable_smooth_line(int enable) {
    if (enable)
        glEnable(GL_LINE_SMOOTH);
    else
        glDisable(GL_LINE_SMOOTH);
}

void graphics_enable_smooth_polygon(int enable) {
    if (enable)
        glEnable(GL_POLYGON_SMOOTH);
    else
        glDisable(GL_POLYGON_SMOOTH);
}

void graphics_enable_scissor(int x, int y, int w, int h) {
    glScissor(x, y, w, h);
    glEnable(GL_SCISSOR_TEST);
}

void graphics_disable_scissor() {
    glDisable(GL_SCISSOR_TEST);
}

void graphics_viewport(int x, int y, int w, int h) {
    glViewport(x, y, w, h);
}

void graphics_clear(float r, float g, float b) {
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void graphics_clear_depth() {
    glClear(GL_DEPTH_BUFFER_BIT);
}

void graphics_point_size(float size) {
    glPointSize(size);
}

void graphics_ortho(mat4 *m, float left, float right, float bottom, float top, float _near, float _far) {
    mat4_set_identity(m);
    m->v[0][0] = 2.0f / (right - left);
    m->v[1][1] = 2.0f / (top - bottom);
    m->v[3][0] = -(right + left) / (right - left);
    m->v[3][1] = -(top + bottom) / (top - bottom);
    m->v[2][2] = -1.0f / (_far - _near);
    m->v[3][2] = -_near / (_far - _near);
}

void graphics_frustum(mat4 *m, float left, float right, float bottom, float top, float near, float far) {
    m->v[0][0] = 2.0f * near / (right - left);
    m->v[0][1] = 0.0f;
    m->v[0][2] = 0.0f;
    m->v[0][3] = 0.0f;
    m->v[1][0] = 0.0f;
    m->v[1][1] = 2.0f * near / (top - bottom);
    m->v[1][2] = 0.0f;
    m->v[1][3] = 0.0f;
    m->v[2][0] = (right + left) / (right - left);
    m->v[2][1] = (top + bottom) / (top - bottom);
    m->v[2][2] = -(far + near) / (far - near);
    m->v[2][3] = -1.0f;
    m->v[3][0] = 0.0f;
    m->v[3][1] = 0.0f;
    m->v[3][2] = -2.0f * far * near / (far - near);
    m->v[3][3] = 0.0f;
}

void graphics_perspective(mat4 *m, float fov, float aspect, float _near, float _far) {
    float left = -_near * tanf(fov * 0.5f);
    float right = -left;
    float bottom = left / aspect;
    float top = -bottom;
    graphics_frustum(m, left, right, bottom, top, _near, _far);
}

void graphics_lookat(mat4 *m, vec3 pos, vec3 fwd, vec3 up) {
    fwd = vec3_normalize(fwd);
    vec3 side = vec3_cross(fwd, up);
    side = vec3_normalize(side);
    up = vec3_cross(side, fwd);
    m->v[0][3] = m->v[1][3] = m->v[2][3] = 0.0f;
    m->v[3][3] = 1;
    m->v[0][0] = side.x;
    m->v[1][0] = side.y;
    m->v[2][0] = side.z;
    m->v[0][1] = up.x;
    m->v[1][1] = up.y;
    m->v[2][1] = up.z;
    m->v[0][2] = -fwd.x;
    m->v[1][2] = -fwd.y;
    m->v[2][2] = -fwd.z;
    m->v[3][0] = -vec3_dot(side, pos);
    m->v[3][1] = -vec3_dot(up, pos);
    m->v[3][2] = vec3_dot(fwd, pos);
}

void graphics_draw_points(int verts_count) {
    glDrawArrays(GL_POINTS, 0, verts_count);
}

void graphics_draw_lines(int verts_count) {
    glDrawArrays(GL_LINES, 0, verts_count);
}

void graphics_draw_triangles(int verts_count) {
    glDrawArrays(GL_TRIANGLES, 0, verts_count);
}

void graphics_draw_quads(int verts_count) {
    glDrawArrays(GL_QUADS, 0, verts_count);
}

void graphics_draw_triangles_indexed(int indices_count, int *indices) {
    glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, indices);
}

void graphics_draw_quads_indexed(int indices_count, int *indices) {
    glDrawElements(GL_QUADS, indices_count, GL_UNSIGNED_INT, indices);
}

void graphics_draw_triangles_instanced(int indices_count, int instances_count) {
    glDrawArraysInstanced(GL_TRIANGLES, 0, indices_count, instances_count);
}

void graphics_draw_quads_instanced(int indices_count, int instances_count) {
    glDrawArraysInstanced(GL_QUADS, 0, indices_count, instances_count);
}

void graphics_read_pixels(int x, int y, int w, int h, unsigned char *rgba) {
    glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

void graphics_read_depths(int x, int y, int w, int h, float *depths) {
    glReadPixels(x, y, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, depths);
}

vec3 vec3_normalize(vec3 v) {
    vec3 r;
    float l = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    r.x = v.x / l;
    r.y = v.y / l;
    r.z = v.z / l;
    return r;
}

float vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vec3_cross(vec3 a, vec3 b) {
    vec3 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

void mat4_set_identity(mat4 *m) {
    m->v[0][0] = 1.0f;
    m->v[0][1] = 0.0f;
    m->v[0][2] = 0.0f;
    m->v[0][3] = 0.0f;

    m->v[1][0] = 0.0f;
    m->v[1][1] = 1.0f;
    m->v[1][2] = 0.0f;
    m->v[1][3] = 0.0f;

    m->v[2][0] = 0.0f;
    m->v[2][1] = 0.0f;
    m->v[2][2] = 1.0f;
    m->v[2][3] = 0.0f;

    m->v[3][0] = 0.0f;
    m->v[3][1] = 0.0f;
    m->v[3][2] = 0.0f;
    m->v[3][3] = 1.0f;
}


void mat4_scale(mat4 *m, float v) {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            m->v[i][j] *= v;
}

void mat4_translate(mat4 *m, float x, float y, float z) {
    m->v[3][0] += m->v[0][0] * x + m->v[1][0] * y + m->v[2][0] * z;
    m->v[3][1] += m->v[0][1] * x + m->v[1][1] * y + m->v[2][1] * z;
    m->v[3][2] += m->v[0][2] * x + m->v[1][2] * y + m->v[2][2] * z;
}

void mat4_rotate(mat4 *m, float a, float x, float y, float z) {
    float l = sqrtf(x * x + y * y + z * z);
    float c = cosf(a), s = sinf(a), _c = 1.0f - c;

    float v[3][3];

    v[0][0] = c + x * x * _c;
    v[0][1] = y * x * _c + z * s;
    v[0][2] = z * x * _c - y * s;

    v[1][0] = x * y * _c - z * s;
    v[1][1] = c + y * y * _c;
    v[1][2] = z * y * _c + x * s;

    v[2][0] = x * z * _c + y * s;
    v[2][1] = y * z * _c - x * s;
    v[2][2] = c + z * z * _c;

    float r[4][4];

    r[0][0] = m->v[0][0] * v[0][0] + m->v[1][0] * v[0][1] + m->v[2][0] * v[0][2];
    r[0][1] = m->v[0][1] * v[0][0] + m->v[1][1] * v[0][1] + m->v[2][1] * v[0][2];
    r[0][2] = m->v[0][2] * v[0][0] + m->v[1][2] * v[0][1] + m->v[2][2] * v[0][2];
    r[0][3] = m->v[0][3] * v[0][0] + m->v[1][3] * v[0][1] + m->v[2][3] * v[0][2];

    r[1][0] = m->v[0][0] * v[1][0] + m->v[1][0] * v[1][1] + m->v[2][0] * v[1][2];
    r[1][1] = m->v[0][1] * v[1][0] + m->v[1][1] * v[1][1] + m->v[2][1] * v[1][2];
    r[1][2] = m->v[0][2] * v[1][0] + m->v[1][2] * v[1][1] + m->v[2][2] * v[1][2];
    r[1][3] = m->v[0][3] * v[1][0] + m->v[1][3] * v[1][1] + m->v[2][3] * v[1][2];

    r[2][0] = m->v[0][0] * v[2][0] + m->v[1][0] * v[2][1] + m->v[2][0] * v[2][2];
    r[2][1] = m->v[0][1] * v[2][0] + m->v[1][1] * v[2][1] + m->v[2][1] * v[2][2];
    r[2][2] = m->v[0][2] * v[2][0] + m->v[1][2] * v[2][1] + m->v[2][2] * v[2][2];
    r[2][3] = m->v[0][3] * v[2][0] + m->v[1][3] * v[2][1] + m->v[2][3] * v[2][2];

    r[3][0] = m->v[3][0];
    r[3][1] = m->v[3][1];
    r[3][2] = m->v[3][2];
    r[3][3] = m->v[3][3];

    memcpy(m->v, r, 16 * sizeof(float));
}

void mat4_multiply(mat4 *r, mat4 *a, mat4 *b) {
    r->v[0][0] = a->v[0][0] * b->v[0][0] + a->v[1][0] * b->v[0][1] + a->v[2][0] * b->v[0][2] + a->v[3][0] * b->v[0][3];
    r->v[0][1] = a->v[0][1] * b->v[0][0] + a->v[1][1] * b->v[0][1] + a->v[2][1] * b->v[0][2] + a->v[3][1] * b->v[0][3];
    r->v[0][2] = a->v[0][2] * b->v[0][0] + a->v[1][2] * b->v[0][1] + a->v[2][2] * b->v[0][2] + a->v[3][2] * b->v[0][3];
    r->v[0][3] = a->v[0][3] * b->v[0][0] + a->v[1][3] * b->v[0][1] + a->v[2][3] * b->v[0][2] + a->v[3][3] * b->v[0][3];

    r->v[1][0] = a->v[0][0] * b->v[1][0] + a->v[1][0] * b->v[1][1] + a->v[2][0] * b->v[1][2] + a->v[3][0] * b->v[1][3];
    r->v[1][1] = a->v[0][1] * b->v[1][0] + a->v[1][1] * b->v[1][1] + a->v[2][1] * b->v[1][2] + a->v[3][1] * b->v[1][3];
    r->v[1][2] = a->v[0][2] * b->v[1][0] + a->v[1][2] * b->v[1][1] + a->v[2][2] * b->v[1][2] + a->v[3][2] * b->v[1][3];
    r->v[1][3] = a->v[0][3] * b->v[1][0] + a->v[1][3] * b->v[1][1] + a->v[2][3] * b->v[1][2] + a->v[3][3] * b->v[1][3];

    r->v[2][0] = a->v[0][0] * b->v[2][0] + a->v[1][0] * b->v[2][1] + a->v[2][0] * b->v[2][2] + a->v[3][0] * b->v[2][3];
    r->v[2][1] = a->v[0][1] * b->v[2][0] + a->v[1][1] * b->v[2][1] + a->v[2][1] * b->v[2][2] + a->v[3][1] * b->v[2][3];
    r->v[2][2] = a->v[0][2] * b->v[2][0] + a->v[1][2] * b->v[2][1] + a->v[2][2] * b->v[2][2] + a->v[3][2] * b->v[2][3];
    r->v[2][3] = a->v[0][3] * b->v[2][0] + a->v[1][3] * b->v[2][1] + a->v[2][3] * b->v[2][2] + a->v[3][3] * b->v[2][3];

    r->v[3][0] = a->v[0][0] * b->v[3][0] + a->v[1][0] * b->v[3][1] + a->v[2][0] * b->v[3][2] + a->v[3][0] * b->v[3][3];
    r->v[3][1] = a->v[0][1] * b->v[3][0] + a->v[1][1] * b->v[3][1] + a->v[2][1] * b->v[3][2] + a->v[3][1] * b->v[3][3];
    r->v[3][2] = a->v[0][2] * b->v[3][0] + a->v[1][2] * b->v[3][1] + a->v[2][2] * b->v[3][2] + a->v[3][2] * b->v[3][3];
    r->v[3][3] = a->v[0][3] * b->v[3][0] + a->v[1][3] * b->v[3][1] + a->v[2][3] * b->v[3][2] + a->v[3][3] * b->v[3][3];
}
