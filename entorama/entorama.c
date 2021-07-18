#include "entorama.h"
#include "tay.h"
#include <string.h>


static void _set_world_box(EntoramaModel *model, float min_x, float min_y, float min_z, float max_x, float max_y, float max_z) {
    model->min_x = min_x;
    model->min_y = min_y;
    model->min_z = min_z;
    model->max_x = max_x;
    model->max_y = max_y;
    model->max_z = max_z;
}

static EntoramaGroup *_add_group(EntoramaModel *model, const char *name, TayGroup *tay_group, unsigned max_agents) {
    EntoramaGroup *group = model->groups + model->groups_count++;
    group->group = tay_group;
    group->max_agents = max_agents;
    strcpy_s(group->name, ENTORAMA_MAX_NAME, name);
    group->position_x_offset = 0;
    group->position_y_offset = 4;
    group->position_z_offset = 8;
    group->direction_source = ENTORAMA_DIRECTION_AUTO;
    group->color_source = ENTORAMA_COLOR_AUTO;
    group->size_source = ENTORAMA_SIZE_AUTO;
    group->shape = ENTORAMA_CUBE;
    return group;
}

void entorama_init_model(EntoramaModel *model) {
    model->init = 0;
    model->groups_count = 0;
    model->set_world_box = _set_world_box;
    model->add_group = _add_group;
    model->min_x = -100.0f;
    model->min_y = -100.0f;
    model->min_z = -100.0f;
    model->max_x = 100.0f;
    model->max_y = 100.0f;
    model->max_z = 100.0f;
}
