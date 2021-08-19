#ifndef graphics_h
#define graphics_h

#define GRAPH_MAX_VBOS      32
#define GRAPH_MAX_UNIFORMS  32


typedef struct vec2 {
    float x, y;
} vec2;

typedef struct vec3 {
    float x, y, z;
} vec3;

typedef struct vec4 {
    float x, y, z, w;
} vec4;

typedef struct mat4 {
    float v[4][4];
} mat4;

typedef struct QuadBuffer {
    unsigned count;
    unsigned capacity;
    struct vec2 *pos;
    struct vec4 *col;
} QuadBuffer;

typedef struct TexQuadBuffer {
    unsigned count;
    unsigned capacity;
    struct vec2 *pos;
    struct vec2 *tex;
    struct vec4 *col;
} TexQuadBuffer;

typedef struct Program {
    unsigned prog, vao;
    unsigned vbos[GRAPH_MAX_VBOS];
    int vbo_count;
    unsigned uniforms[GRAPH_MAX_UNIFORMS];
    int uniforms_count;
} Program;

void shader_program_init(Program *p, const char *vert_src, const char *vert_title, const char *vert_defines, const char *frag_src, const char *frag_title, const char *frag_defines);
void shader_program_use(Program *p);
void shader_program_define_uniform(Program *p, const char *name);
void shader_program_set_uniform_mat4(Program *p, int uniform_index, mat4 *mat);
void shader_program_set_uniform_vec4(Program *p, int uniform_index, vec4 *vec);
void shader_program_set_uniform_vec3(Program *p, int uniform_index, vec3 *vec);

void shader_program_define_in_float(Program *p, int components);
void shader_program_define_instanced_in_float(Program *p, int components);
void shader_program_set_data_float(Program *p, int vbo_index, int count, int components, void *data);

void graphics_enable_depth_test(int enable);
void graphics_enable_blend(int enable);
void graphics_enable_smooth_line(int enable);
void graphics_enable_smooth_polygon(int enable);

void graphics_draw_points(int verts_count);
void graphics_draw_lines(int verts_count);
void graphics_draw_triangles(int verts_count);
void graphics_draw_quads(int verts_count);
void graphics_draw_triangles_indexed(int indices_count, int *indices);
void graphics_draw_quads_indexed(int indices_count, int *indices);
void graphics_draw_triangles_instanced(int indices_count, int instances_count);
void graphics_draw_quads_instanced(int indices_count, int instances_count);

void graphics_read_pixels(int x, int y, int w, int h, unsigned char *rgba);
void graphics_read_depths(int x, int y, int w, int h, float *depths);

void graphics_enable_scissor(int x, int y, int w, int h);
void graphics_disable_scissor();
void graphics_viewport(int x, int y, int w, int h);
void graphics_clear(float r, float g, float b);
void graphics_clear_depth();
void graphics_point_size(float size);

void graphics_ortho(mat4 *m, float left, float right, float bottom, float top, float near, float far);
void graphics_frustum(mat4 *m, float left, float right, float bottom, float top, float near, float far);
void graphics_perspective(mat4 *m, float fov, float aspect, float near, float far);
void graphics_lookat(mat4 *m, vec3 pos, vec3 fwd, vec3 up);

vec3 vec3_normalize(vec3 v);
float vec3_dot(vec3 a, vec3 b);
vec3 vec3_cross(vec3 a, vec3 b);

void mat4_set_identity(mat4 *m);
void mat4_scale(mat4 *m, float v);
void mat4_translate(mat4 *m, float x, float y, float z);
void mat4_rotate(mat4 *m, float angle, float x, float y, float z);
void mat4_multiply(mat4 *r, mat4 *a, mat4 *b);

void quad_buffer_clear(QuadBuffer *buffer);
void quad_buffer_add(QuadBuffer *buffer, unsigned count, vec2 **pos, vec4 **col);

void tex_quad_buffer_clear(TexQuadBuffer *buffer);
void tex_quad_buffer_add(TexQuadBuffer *buffer, unsigned count, vec2 **pos, vec2 **tex, vec4 **col);

unsigned graphics_create_buffer(unsigned location, unsigned max_count, unsigned components);
unsigned graphics_create_buffer_instanced(unsigned location, unsigned max_count, unsigned components);
void graphics_copy_to_buffer(unsigned buffer_id, void *data, unsigned count, unsigned components);

void graphics_print_error();

#endif
