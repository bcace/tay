#include "font.h"
#include "font_raster.h"
#include "shaders.h"
#include "graphics.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <string.h>


Font inconsolata13;

static unsigned HORZ_SPACING = 0;

static void _create_texture(Font *font, FontRaster *raster) {
    font->w = raster->w;
    font->h = raster->h;
    font->nw = raster->w / (float)raster->iw;
    font->nh = raster->h / (float)raster->ih;

    glGenTextures(1, &font->id);
    glBindTexture(GL_TEXTURE_2D, font->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, raster->iw, raster->ih, 0, GL_RGBA, GL_UNSIGNED_BYTE, raster->data);
}

void font_init() {
    FontRaster *raster = font_raster_get();

    font_raster_create_inconsolata13();
    _create_texture(&inconsolata13, raster);

    font_raster_clear();
}

static Font *_font_for_size(FontSize font_size) {
    if (font_size == EM_FONT_MEDIUM)
        return &inconsolata13;
    else
        return 0;
}

void font_use_medium() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _font_for_size(EM_FONT_MEDIUM)->id);
}

void font_draw_text(FontSize font_size, const char *text, int x, int y, vec4 color, TexQuadBuffer *buffer) {
    Font *font = _font_for_size(font_size);

    unsigned text_size = (unsigned)strlen(text);

    vec2 *pos = 0;
    vec2 *tex = 0;
    vec4 *col = 0;
    tex_quad_buffer_add(buffer, text_size, &pos, &tex, &col);

    for (unsigned char_i = 0; char_i < text_size; ++char_i) {
        unsigned vert_i = char_i * 4;

        float qx = (float)(x + (font->w + HORZ_SPACING) * char_i);
        float qy = (float)y;
        pos[0].x = qx;
        pos[0].y = qy;
        pos[1].x = qx + font->w;
        pos[1].y = qy;
        pos[2].x = qx + font->w;
        pos[2].y = qy + font->h;
        pos[3].x = qx;
        pos[3].y = qy + font->h;
        pos += 4;

        float tx = (text[char_i] - 33) * font->nw;
        float ty = 0.0f; // TODO: ???
        tex[0].x = tx;
        tex[0].y = ty;
        tex[1].x = tx + font->nw;
        tex[1].y = ty;
        tex[2].x = tx + font->nw;
        tex[2].y = ty + font->nh;
        tex[3].x = tx;
        tex[3].y = ty + font->nh;
        tex += 4;

        col[0] = color;
        col[1] = color;
        col[2] = color;
        col[3] = color;
        col += 4;
    }
}

unsigned font_text_width(FontSize font_size, const char *text) {
    if (font_size == EM_FONT_MEDIUM)
        return (unsigned)strlen(text) * (inconsolata13.w + HORZ_SPACING);
    else
        return 0;
}

unsigned font_text_height(FontSize font_size, const char *text) {
    if (font_size == EM_FONT_MEDIUM)
        return inconsolata13.h;
    else
        return 0;
}

unsigned font_height(FontSize font_size) {
    if (font_size == EM_FONT_MEDIUM)
        return inconsolata13.h;
    else
        return 0;
}
