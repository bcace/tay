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
    ENTORAMA_FONT_MEDIUM,
} FontSize;

typedef struct FontTextBuffer {
    unsigned count; /* character quads */
    unsigned capacity; /* character quads */
    struct vec2 *pos;
    struct vec2 *tex;
    struct vec4 *col;
} FontTextBuffer;

void font_init();
void font_use_medium();
void font_draw_text(const char *text, int x, int y, struct vec4 color, FontTextBuffer *buffer);
unsigned font_text_width(FontSize font_size, const char *text);
unsigned font_text_height(FontSize font_size, const char *text);
unsigned font_height(FontSize font_size);

void font_text_buffer_clear(FontTextBuffer *buffer);

#endif
