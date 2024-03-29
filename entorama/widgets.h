#ifndef widgets_h
#define widgets_h


typedef enum EmWidgetFlags {
    EM_WIDGET_FLAGS_NONE = 0x0,
    EM_WIDGET_FLAGS_PRESSED = 0x1,
    EM_WIDGET_FLAGS_DISABLED = 0x2,
    EM_WIDGET_FLAGS_ICON_ONLY = 0x4,
} EmWidgetFlags;

typedef enum EmResponse {
    EM_RESPONSE_NONE,
    EM_RESPONSE_CLICKED,
    EM_RESPONSE_HOVERED,
    EM_RESPONSE_PRESSED,
} EmResponse;

void em_widgets_init();
void em_widgets_begin();
void em_widgets_end();
void em_widgets_draw(struct mat4 projection);

EmResponse em_button(char *label, float min_x, float min_y, float max_x, float max_y, EmWidgetFlags flags);
EmResponse em_button_with_icon(char *label, unsigned index, float min_x, float min_y, float max_x, float max_y, EmWidgetFlags flags);
EmResponse em_label(char *label, float min_x, float min_y, float max_x, float max_y, EmWidgetFlags flags);

void em_quad(float min_x, float min_y, float max_x, float max_y, struct vec4 color);

void em_select_layer(unsigned layer_index);
void em_set_layer_scissor(float min_x, float min_y, float max_x, float max_y);

int em_widget_pressed();

extern int mouse_l;
extern int mouse_r;
extern float mouse_x;
extern float mouse_y;
extern int redraw;

#endif
