#include "entorama.h"
#include "tay.h"
#include <string.h>


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
    model->add_group = _add_group;
}
