#ifndef entorama_h
#define entorama_h

#define ENTORAMA_MAX_GROUPS     256


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

typedef struct EntoramaGroupInfo {
    struct TayGroup *group;
    unsigned max_agents;

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
} EntoramaGroupInfo;

typedef struct EntoramaSimulationInfo {
    EntoramaGroupInfo groups[ENTORAMA_MAX_GROUPS];
    unsigned groups_count;
} EntoramaSimulationInfo;

typedef int (*ENTORAMA_INIT)(EntoramaSimulationInfo *info, struct TayState *);

typedef struct EntoramaModelInfo {
    ENTORAMA_INIT init;
    float origin_x, origin_y, origin_z;
    float radius;
} EntoramaModelInfo;

typedef int (*ENTORAMA_MAIN)(EntoramaModelInfo *);

#endif
