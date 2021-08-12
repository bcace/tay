#ifndef entorama_h
#define entorama_h

#define EM_MAX_GROUPS     256
#define EM_MAX_PASSES     256
#define EM_MAX_NAME       256


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

typedef enum EmShape {
    EM_CUBE,
    EM_PYRAMID,
    EM_SPHERE,
} EmShape;

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
    void (*configure_space)(struct EmGroup *group, enum TaySpaceType space_type, float min_part_size_x, float min_part_size_y, float min_part_size_z, float min_part_size_w);

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

    EmShape shape;

    /* widget stuff */
    int expanded;
    int structures_expanded;
} EmGroup;

typedef enum EmPassType {
    EM_PASS_ACT,
    EM_PASS_SEE,
} EmPassType;

typedef struct EmPass {
    EmPassType type;
    char name[EM_MAX_NAME];
    union {
        EmGroup *act_group;
        EmGroup *seer_group;
    };
    EmGroup *seen_group;

    /* widget stuff */
    int expanded;
} EmPass;

typedef struct EmModel {
    int ocl_enabled;

    /* filled by member functions */
    EmGroup groups[EM_MAX_GROUPS];
    unsigned groups_count;
    EmPass passes[EM_MAX_PASSES];
    unsigned passes_count;
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;

    /* member functions */
    void (*set_world_box)(struct EmModel *model, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z);
    EmGroup *(*add_group)(struct EmModel *model, const char *name, struct TayGroup *group, unsigned max_agents, int is_point);
    EmPass *(*add_see)(struct EmModel *model, const char *name, EmGroup *seer_group, EmGroup *seen_group);
    EmPass *(*add_act)(struct EmModel *model, const char *name, EmGroup *group);
} EmModel;

typedef struct EmIface {
    int (*init)(struct EmModel *model, struct TayState *tay);
    int (*reset)(struct EmModel *model, struct TayState *tay);
} EmIface;

typedef int (*EM_MAIN)(EmIface *iface);

#endif
