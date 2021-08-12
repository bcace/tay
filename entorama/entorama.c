#include "entorama.h"
#include "tay.h"
#include "platform.h"
#include <string.h>


static void _set_world_box(EmModel *model, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z) {
    model->min_x = min_x;
    model->min_y = min_y;
    model->min_z = min_z;
    model->max_x = max_x;
    model->max_y = max_y;
    model->max_z = max_z;
}

static void _configure_space(EmGroup *group, TaySpaceType space_type, float min_part_size_x, float min_part_size_y, float min_part_size_z, float min_part_size_w) {
    group->space_type = space_type;
    group->min_part_size_x = min_part_size_x;
    group->min_part_size_y = min_part_size_y;
    group->min_part_size_z = min_part_size_z;
    group->min_part_size_w = min_part_size_w;
}

static EmGroup *_add_group(EmModel *model, const char *name, TayGroup *tay_group, unsigned max_agents, int is_point) {
    EmGroup *group = model->groups + model->groups_count++;
    strcpy_s(group->name, EM_MAX_NAME, name);
    group->group = tay_group;
    group->max_agents = max_agents;
    group->position_x_offset = 0;
    group->position_y_offset = 4;
    group->position_z_offset = 8;
    group->direction_source = EM_DIRECTION_AUTO;
    group->color_source = EM_COLOR_AUTO;
    group->size_source = EM_SIZE_AUTO;
    group->shape = EM_CUBE;
    group->is_point = is_point;
    group->configure_space = _configure_space;
    group->expanded = 0;
    group->structures_expanded = 0;
    return group;
}

EmPass *_add_see(EmModel *model, const char *name, EmGroup *seer_group, EmGroup *seen_group) {
    EmPass *pass = model->passes + model->passes_count++;
    strcpy_s(pass->name, EM_MAX_NAME, name);
    pass->type = EM_PASS_SEE;
    pass->seer_group = seer_group;
    pass->seen_group = seen_group;
    pass->expanded = 0;
    return pass;
}

EmPass *_add_act(EmModel *model, const char *name, EmGroup *group) {
    EmPass *pass = model->passes + model->passes_count++;
    strcpy_s(pass->name, EM_MAX_NAME, name);
    pass->type = EM_PASS_ACT;
    pass->act_group = group;
    pass->expanded = 0;
    return pass;
}

void entorama_load_model_dll(EmModel *model, char *path) {
    model->init = 0;
    model->groups_count = 0;
    model->passes_count = 0;
    model->set_world_box = _set_world_box;
    model->add_group = _add_group;
    model->add_see = _add_see;
    model->add_act = _add_act;
    model->min_x = -100.0f;
    model->min_y = -100.0f;
    model->min_z = -100.0f;
    model->max_x = 100.0f;
    model->max_y = 100.0f;
    model->max_z = 100.0f;
    model->ocl_enabled = 0;
    void *lib = platform_load_library(path);
    EM_MAIN entorama_main = platform_load_library_function(lib, "entorama_main");
    int r = entorama_main(model);
}
