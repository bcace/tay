#include "font.h"
#include "font_raster.h"
#include "shaders.h"
#include "graphics.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>
#include <string.h>


Font font_medium;
Font icons_medium;

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

    font_raster_create_iosevka7x16();
    _create_texture(&font_medium, raster);

    font_raster_create_icons16x16();
    _create_texture(&icons_medium, raster);

    font_raster_clear();
}

static Font *_font_for_size(FontSize font_size) {
    if (font_size == EM_FONT_MEDIUM)
        return &font_medium;
    else
        return 0;
}

static Font *_icons_for_size(FontSize font_size) {
    if (font_size == EM_FONT_MEDIUM)
        return &icons_medium;
    else
        return 0;
}

void font_use_medium() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _font_for_size(EM_FONT_MEDIUM)->id);
}

void icons_use_medium() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _icons_for_size(EM_FONT_MEDIUM)->id);
}

void font_draw_text(FontSize font_size, const char *text, int x, int y, vec4 color, TexQuadBuffer *buffer) {
    Font *font = _font_for_size(font_size);

    unsigned text_size = (unsigned)strlen(text);

    vec2 *pos = 0;
    vec2 *tex = 0;
    vec4 *col = 0;
    tex_quad_buffer_add(buffer, text_size, &pos, &tex, &col);

    for (unsigned char_i = 0; char_i < text_size; ++char_i) {
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

void font_draw_icon(FontSize font_size, unsigned index, int x, int y, struct vec4 color, struct TexQuadBuffer *buffer) {
    Font *font = _icons_for_size(font_size);

    vec2 *pos = 0;
    vec2 *tex = 0;
    vec4 *col = 0;
    tex_quad_buffer_add(buffer, 1, &pos, &tex, &col);

    {
        float qx = (float)x;
        float qy = (float)y;
        pos[0].x = qx;
        pos[0].y = qy;
        pos[1].x = qx + font->w;
        pos[1].y = qy;
        pos[2].x = qx + font->w;
        pos[2].y = qy + font->h;
        pos[3].x = qx;
        pos[3].y = qy + font->h;

        float tx = index * font->nw;
        float ty = 0.0f; // TODO: ???
        tex[0].x = tx;
        tex[0].y = ty;
        tex[1].x = tx + font->nw;
        tex[1].y = ty;
        tex[2].x = tx + font->nw;
        tex[2].y = ty + font->nh;
        tex[3].x = tx;
        tex[3].y = ty + font->nh;

        col[0] = color;
        col[1] = color;
        col[2] = color;
        col[3] = color;
    }
}

unsigned font_text_width(FontSize font_size, const char *text) {
    if (font_size == EM_FONT_MEDIUM)
        return (unsigned)strlen(text) * (font_medium.w + HORZ_SPACING);
    else
        return 0;
}

unsigned font_text_height(FontSize font_size, const char *text) {
    if (font_size == EM_FONT_MEDIUM)
        return font_medium.h;
    else
        return 0;
}

unsigned font_height(FontSize font_size) {
    if (font_size == EM_FONT_MEDIUM)
        return font_medium.h;
    else
        return 0;
}

unsigned icons_size(FontSize font_size) {
    if (font_size == EM_FONT_MEDIUM)
        return icons_medium.h;
    else
        return 0;
}
