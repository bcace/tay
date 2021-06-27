#ifndef main_h
#define main_h


/* colors.c */
struct vec4 color_bg();
struct vec4 color_fg();
struct vec4 color_palette(int index);

/* model.c */
void model_load(struct EntoramaModelInfo *info, char *path);

/* shapes.c */
float PYRAMID_VERTS[];
int PYRAMID_VERTS_COUNT;
float ICOSAHEDRON_VERTS[];
int ICOSAHEDRON_VERTS_COUNT;
float CUBE_VERTS[];
int CUBE_VERTS_COUNT;
int icosahedron_verts_count(unsigned subdivs);
void icosahedron_verts(unsigned subdivs, float *verts);

#endif
