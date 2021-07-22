#ifndef widgets_h
#define widgets_h


typedef enum EmButtonState {
    EM_BUTTON_STATE_NONE,
    EM_BUTTON_STATE_PRESSED,
    EM_BUTTON_STATE_DISABLED,
} EmButtonState;

typedef enum EmResponse {
    EM_RESPONSE_NONE,
    EM_RESPONSE_CLICKED,
    EM_RESPONSE_HOVERED,
    EM_RESPONSE_PRESSED,
} EmResponse;

void em_widgets_init();
void em_widgets_begin();
void em_widgets_end(struct mat4 projection);

EmResponse em_button(char *label, float min_x, float min_y, float max_x, float max_y, EmButtonState state);
EmResponse em_area(char *label, float min_x, float min_y, float max_x, float max_y, struct vec4 color);
EmResponse em_label(char *label, float min_x, float min_y, float max_x, float max_y);

int em_widget_pressed();

extern int mouse_l;
extern int mouse_r;
extern float mouse_x;
extern float mouse_y;

#endif
