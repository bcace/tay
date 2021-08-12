#ifndef main_h
#define main_h


/* drawing.c */
void drawing_init(int max_agents_per_group);
void drawing_mouse_scroll(double y, int *redraw);
void drawing_mouse_move(int button_l, int button_r, float dx, float dy, int *execute_draw_calls);
void drawing_camera_setup(struct EmModel *model, int window_w, int window_h);
void drawing_draw_group(struct TayState *tay, struct EmGroup *group, int group_i);
void drawing_update_world_box(struct EmModel *model);

/* colors.c */
void color_init();
struct vec4 color_border();
struct vec4 color_vd();
struct vec4 color_bg();
struct vec4 color_fg();
struct vec4 color_fg_hover();
struct vec4 color_fg_disabled();
struct vec4 color_hi();
struct vec4 color_palette(int index);
void color_toggle_theme();

/* model.c */
void model_load(struct EmModel *model, char *path);

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
void entorama_load_model_dll(struct EmModel *model, char *path);

#endif
