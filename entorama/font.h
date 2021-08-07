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
    EM_FONT_MEDIUM,
} FontSize;

void font_init();
void font_use_medium();
void icons_use_medium();
void font_draw_text(FontSize font_size, const char *text, int x, int y, struct vec4 color, struct TexQuadBuffer *buffer);
void font_draw_icon(FontSize font_size, unsigned index, int x, int y, struct vec4 color, struct TexQuadBuffer *buffer);
unsigned font_text_width(FontSize font_size, const char *text);
unsigned font_text_height(FontSize font_size, const char *text);
unsigned font_height(FontSize font_size);
unsigned font_width(FontSize font_size);
unsigned icons_size(FontSize font_size);

#endif
