#include "font.h"
#include "font_raster.h"
#include "shaders.h"
#include "graphics.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <string.h>


Font inconsolata13;
Font *font; /* currently used font */

static unsigned HORZ_SPACING = 1;

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

void font_use_medium() {
    font = &inconsolata13;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->id);
}

void font_text_buffer_clear(FontTextBuffer *buffer) {
    buffer->count = 0;
}

static void _ensure_text_buffer_capacity(FontTextBuffer *buffer, unsigned size) {
    if (buffer->count + size > buffer->capacity) {
        buffer->capacity = buffer->count + size + 64;
        buffer->pos = realloc(buffer->pos, buffer->capacity * sizeof(vec2) * 4);
        buffer->tex = realloc(buffer->tex, buffer->capacity * sizeof(vec2) * 4);
        buffer->col = realloc(buffer->col, buffer->capacity * sizeof(vec4) * 4);
    }
}

void font_draw_text(const char *text, int x, int y, vec4 color, FontTextBuffer *buffer) {
    unsigned text_size = (unsigned)strlen(text);

    _ensure_text_buffer_capacity(buffer, text_size);
    vec2 *pos = buffer->pos + buffer->count * 4;
    vec2 *tex = buffer->tex + buffer->count * 4;
    vec4 *col = buffer->col + buffer->count * 4;
    buffer->count += text_size;

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
    if (font_size == ENTORAMA_FONT_MEDIUM)
        return (unsigned)strlen(text) * (inconsolata13.w + HORZ_SPACING);
    else
        return 0;
}

unsigned font_text_height(FontSize font_size, const char *text) {
    if (font_size == ENTORAMA_FONT_MEDIUM)
        return inconsolata13.h;
    else
        return 0;
}

unsigned font_height(FontSize font_size) {
    if (font_size == ENTORAMA_FONT_MEDIUM)
        return inconsolata13.h;
    else
        return 0;
}
