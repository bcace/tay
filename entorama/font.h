#ifndef font_h
#define font_h


typedef struct Font {
    unsigned id;
    unsigned w;
    unsigned h;
    float nw;
    float nh;
} Font;

typedef enum FontSize {
    ENTORAMA_FONT_MEDIUM
} FontSize;

void font_init();
void font_use_medium();
void font_draw_text(const char *text, int x, int y, struct mat4 projection, struct vec4 color);
unsigned font_text_width(FontSize font_size, const char *text);
unsigned font_text_height(FontSize font_size, const char *text);
unsigned font_height(FontSize font_size);

#endif
