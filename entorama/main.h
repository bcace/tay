#ifndef main_h
#define main_h

#define EM_MAX_GROUPS     256
#define EM_MAX_PASSES     256
#define EM_MAX_NAME       256


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
void entorama_load_model_dll(struct EmIface *iface, struct EmModel *model, char *path);

typedef enum EmColorSource {
    EM_COLOR_AUTO,
    EM_COLOR_UNIFORM_PALETTE,
    EM_COLOR_UNIFORM_RGB,
    EM_COLOR_AGENT_PALETTE,
    EM_COLOR_AGENT_RGB,
} EmColorSource;

typedef enum EmSizeSource {
    EM_SIZE_AUTO,
    EM_SIZE_UNIFORM_RADIUS,
    EM_SIZE_UNIFORM_XYZ,
    EM_SIZE_AGENT_RADIUS,
    EM_SIZE_AGENT_XYZ,
} EmSizeSource;

typedef enum EmDirectionSource {
    EM_DIRECTION_AUTO,
    EM_DIRECTION_FWD,
} EmDirectionSource;

typedef enum EmPassType {
    EM_PASS_ACT,
    EM_PASS_SEE,
} EmPassType;

typedef struct EmGroup {
    struct TayGroup *group;
    unsigned max_agents;
    char name[EM_MAX_NAME];
    int is_point;

    enum TaySpaceType space_type;
    float min_part_size_x;
    float min_part_size_y;
    float min_part_size_z;
    float min_part_size_w;

    /* drawing info */

    struct {
        unsigned position_x_offset, position_y_offset, position_z_offset;
    };

    EmDirectionSource direction_source;
    struct {
        unsigned direction_fwd_x_offset, direction_fwd_y_offset, direction_fwd_z_offset;
    };

    EmColorSource color_source;
    union {
        unsigned palette_index;                 /* EM_COLOR_UNIFORM_PALETTE */
        struct {                                /* EM_COLOR_UNIFORM_RGB */
            float red, green, blue;
        };
        unsigned color_palette_index_offset;    /* EM_COLOR_AGENT_PALETTE */
        struct {                                /* EM_COLOR_AGENT_RGB */
            unsigned color_r_offset, color_g_offset, color_b_offset;
        };
    };

    EmSizeSource size_source;
    union {
        float size_radius;                      /* EM_SIZE_UNIFORM_RADIUS */
        struct {                                /* EM_SIZE_UNIFORM_XYZ */
            float size_x, size_y, size_z;
        };
        unsigned size_radius_offset;            /* EM_SIZE_AGENT_RADIUS */
        struct {                                /* EM_SIZE_AGENT_XYZ */
            unsigned size_x_offset, size_y_offset, size_z_offset;
        };
    };

    enum EmShape shape;

    /* widget related */
    int expanded;
    int structures_expanded;
} EmGroup;

typedef struct EmPass {
    EmPassType type;
    char name[EM_MAX_NAME];
    union {
        EmGroup *act_group;
        EmGroup *seer_group;
    };
    EmGroup *seen_group;

    /* widget related */
    int expanded;
} EmPass;

typedef struct EmModel {
    EmGroup groups[EM_MAX_GROUPS];
    unsigned groups_count;
    EmPass passes[EM_MAX_PASSES];
    unsigned passes_count;
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;

    /* private */
    int ocl_enabled;
} EmModel;

#endif
