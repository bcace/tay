#ifndef font_h
#define font_h


typedef struct Font {
    unsigned id;
    unsigned w;
    unsigned h;
    float nw;
    float nh;
} Font;

void font_init();
void font_use_medium();
void font_draw_text(const char *text, int x, int y, struct mat4 projection, struct vec4 color);
unsigned font_text_length(const char *text);
unsigned font_height();

#endif
