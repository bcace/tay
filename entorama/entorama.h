#ifndef entorama_h
#define entorama_h


typedef enum EmShape {
    EM_CUBE,
    EM_PYRAMID,
    EM_SPHERE,
} EmShape;

typedef struct EmModel EmModel;
typedef struct EmGroup EmGroup;
typedef struct EmPass EmPass;

typedef struct EmIface {
    /* model interface functions */
    int (*init)(EmModel *model, struct TayState *tay);
    int (*reset)(EmModel *model, struct TayState *tay);

    /* entorama interface functions */
    void (*set_world_box)(EmModel *model, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z);
    void (*configure_space)(EmGroup *group, enum TaySpaceType space_type, float min_part_size_x, float min_part_size_y, float min_part_size_z, float min_part_size_w);
    void (*set_shape)(EmGroup *group, EmShape shape);
    void (*set_size_uniform_radius)(EmGroup *group, float radius);
    void (*set_color_agent_palette)(EmGroup *group, unsigned palette_index_offset);
    void (*set_direction_forward)(EmGroup *group, unsigned fwd_x_offset, unsigned fwd_y_offset, unsigned fwd_z_offset);
    EmGroup *(*add_group)(EmModel *model, const char *name, struct TayGroup *group, unsigned max_agents, int is_point);
    EmPass *(*add_see)(EmModel *model, const char *name, EmGroup *seer_group, EmGroup *seen_group);
    EmPass *(*add_act)(EmModel *model, const char *name, EmGroup *group);
} EmIface;

#endif
