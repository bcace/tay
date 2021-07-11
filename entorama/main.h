#ifndef main_h
#define main_h


/* drawing.c */
void drawing_init(int max_agents_per_group);
void drawing_mouse_scroll(double y);
void drawing_mouse_move(int button_l, int button_r, float dx, float dy);
void drawing_camera_setup(struct EntoramaModel *model, int window_w, int window_h);
void drawing_draw_group(struct TayState *tay, struct EntoramaGroup *group, int group_i);

/* colors.c */
struct vec4 color_bg();
struct vec4 color_fg();
struct vec4 color_palette(int index);

/* model.c */
void model_load(struct EntoramaModel *model, char *path);

/* shapes.c */
float PYRAMID_VERTS[];
int PYRAMID_VERTS_COUNT;
float ICOSAHEDRON_VERTS[];
int ICOSAHEDRON_VERTS_COUNT;
float CUBE_VERTS[];
int CUBE_VERTS_COUNT;
int icosahedron_verts_count(unsigned subdivs);
void icosahedron_verts(unsigned subdivs, float *verts);

/* entorama.c */
void entorama_init_model(struct EntoramaModel *model);

#endif
