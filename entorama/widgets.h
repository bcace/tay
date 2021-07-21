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
} EmResponse;

void em_widgets_begin();
void em_widgets_end(struct mat4 projection);

EmResponse em_button(char *label, float min_x, float min_y, float max_x, float max_y, EmButtonState state);
EmResponse em_area(char *label, float min_x, float min_y, float max_x, float max_y);
EmResponse em_label(char *label, float min_x, float min_y, float max_x, float max_y);

#endif
