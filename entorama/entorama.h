#ifndef entorama_h
#define entorama_h

#define ENTORAMA_MAX_GROUPS     256
#define ENTORAMA_MAX_PASSES     256
#define ENTORAMA_MAX_NAME       256


typedef enum EntoramaColorSource {
    ENTORAMA_COLOR_AUTO,
    ENTORAMA_COLOR_UNIFORM_PALETTE,
    ENTORAMA_COLOR_UNIFORM_RGB,
    ENTORAMA_COLOR_AGENT_PALETTE,
    ENTORAMA_COLOR_AGENT_RGB,
} EntoramaColorSource;

typedef enum EntoramaSizeSource {
    ENTORAMA_SIZE_AUTO,
    ENTORAMA_SIZE_UNIFORM_RADIUS,
    ENTORAMA_SIZE_UNIFORM_XYZ,
    ENTORAMA_SIZE_AGENT_RADIUS,
    ENTORAMA_SIZE_AGENT_XYZ,
} EntoramaSizeSource;

typedef enum EntoramaDirectionSource {
    ENTORAMA_DIRECTION_AUTO,
    ENTORAMA_DIRECTION_FWD,
} EntoramaDirectionSource;

typedef enum EntoramaShape {
    ENTORAMA_CUBE,
    ENTORAMA_PYRAMID,
    ENTORAMA_SPHERE,
} EntoramaShape;

typedef struct EntoramaGroup {
    struct TayGroup *group;
    unsigned max_agents;
    char name[ENTORAMA_MAX_NAME];

    enum TaySpaceType space_type;
    float min_part_size_x;
    float min_part_size_y;
    float min_part_size_z;
    float min_part_size_w;
    void (*configure_space)(struct EntoramaGroup *group, enum TaySpaceType space_type, float min_part_size_x, float min_part_size_y, float min_part_size_z, float min_part_size_w);

    /* drawing info */

    struct {
        unsigned position_x_offset, position_y_offset, position_z_offset;
    };

    EntoramaDirectionSource direction_source;
    struct {
        unsigned direction_fwd_x_offset, direction_fwd_y_offset, direction_fwd_z_offset;
    };

    EntoramaColorSource color_source;
    union {
        unsigned palette_index;                 /* ENTORAMA_COLOR_UNIFORM_PALETTE */
        struct {                                /* ENTORAMA_COLOR_UNIFORM_RGB */
            float red, green, blue;
        };
        unsigned color_palette_index_offset;    /* ENTORAMA_COLOR_AGENT_PALETTE */
        struct {                                /* ENTORAMA_COLOR_AGENT_RGB */
            unsigned color_r_offset, color_g_offset, color_b_offset;
        };
    };

    EntoramaSizeSource size_source;
    union {
        float size_radius;                      /* ENTORAMA_SIZE_UNIFORM_RADIUS */
        struct {                                /* ENTORAMA_SIZE_UNIFORM_XYZ */
            float size_x, size_y, size_z;
        };
        unsigned size_radius_offset;            /* ENTORAMA_SIZE_AGENT_RADIUS */
        struct {                                /* ENTORAMA_SIZE_AGENT_XYZ */
            unsigned size_x_offset, size_y_offset, size_z_offset;
        };
    };

    EntoramaShape shape;
} EntoramaGroup;

typedef enum EntoramaPassType {
    ENTORAMA_PASS_ACT,
    ENTORAMA_PASS_SEE,
} EntoramaPassType;


typedef struct EntoramaPass {
    EntoramaPassType type;
    char name[ENTORAMA_MAX_NAME];
    union {
        EntoramaGroup *act_group;
        EntoramaGroup *seer_group;
    };
    EntoramaGroup *seen_group;
} EntoramaPass;

typedef int (*ENTORAMA_INIT_MODEL)(struct EntoramaModel *model, struct TayState *tay);
typedef int (*ENTORAMA_RESET)(struct EntoramaModel *model, struct TayState *tay);

typedef void (*ENTORAMA_SET_WORLD_BOX)(struct EntoramaModel *model, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z);
typedef EntoramaGroup *(*ENTORAMA_ADD_GROUP)(struct EntoramaModel *model, const char *name, struct TayGroup *group, unsigned max_agents);
typedef EntoramaPass *(*ENTORAMA_ADD_SEE)(struct EntoramaModel *model, const char *name, EntoramaGroup *seer_group, EntoramaGroup *seen_group);
typedef EntoramaPass *(*ENTORAMA_ADD_ACT)(struct EntoramaModel *model, const char *name, EntoramaGroup *group);

typedef struct EntoramaModel {
    ENTORAMA_INIT_MODEL init;
    ENTORAMA_RESET reset;
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;

    /* filled by member functions */
    EntoramaGroup groups[ENTORAMA_MAX_GROUPS];
    unsigned groups_count;
    EntoramaPass passes[ENTORAMA_MAX_PASSES];
    unsigned passes_count;

    /* member functions */
    ENTORAMA_SET_WORLD_BOX set_world_box;
    ENTORAMA_ADD_GROUP add_group;
    ENTORAMA_ADD_SEE add_see;
    ENTORAMA_ADD_ACT add_act;
} EntoramaModel;

typedef int (*ENTORAMA_MAIN)(EntoramaModel *model);

#endif
