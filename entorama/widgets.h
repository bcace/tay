#ifndef widgets_h
#define widgets_h


void widgets_begin();
void widgets_end(struct mat4 projection);

int widgets_button(char *label, float min_x, float min_y, float max_x, float max_y);
int widgets_toggle_button(char *label, float min_x, float min_y, float max_x, float max_y, int toggled);

#endif
