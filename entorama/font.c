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

static Program prog;
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

    /* initialize text shader program */
    shader_program_init(&prog, text_vert, "text.vert", "", text_frag, "text.frag", "");
    shader_program_define_in_float(&prog, 2);            /* vertex position */
    shader_program_define_in_float(&prog, 2);            /* vertex texture position */
    shader_program_define_uniform(&prog, "projection");
    shader_program_define_uniform(&prog, "uniform_color");
}

void font_use_medium() {
    font = &inconsolata13;
    shader_program_use(&prog);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font->id);
}

void font_draw_text(const char *text, int x, int y, mat4 projection, vec4 color) {
    unsigned text_size = (unsigned)strlen(text);

    unsigned text_cap = 0;
    static vec2 *pos = 0;
    static vec2 *tex_pos = 0;
    if (text_cap < text_size) {
        text_cap = text_size;
        pos = realloc(pos, text_cap * sizeof(vec2) * 4);
        tex_pos = realloc(tex_pos, text_cap * sizeof(vec2) * 4);
    }

    for (unsigned char_i = 0; char_i < text_size; ++char_i) {
        unsigned vert_i = char_i * 4;

        float qx = (float)(x + (font->w + HORZ_SPACING) * char_i);
        float qy = (float)y;
        pos[vert_i + 0].x = qx;
        pos[vert_i + 0].y = qy;
        pos[vert_i + 1].x = qx + font->w;
        pos[vert_i + 1].y = qy;
        pos[vert_i + 2].x = qx + font->w;
        pos[vert_i + 2].y = qy + font->h;
        pos[vert_i + 3].x = qx;
        pos[vert_i + 3].y = qy + font->h;

        float tx = (text[char_i] - 33) * font->nw;
        float ty = 0.0f; // TODO: ???
        tex_pos[vert_i + 0].x = tx;
        tex_pos[vert_i + 0].y = ty;
        tex_pos[vert_i + 1].x = tx + font->nw;
        tex_pos[vert_i + 1].y = ty;
        tex_pos[vert_i + 2].x = tx + font->nw;
        tex_pos[vert_i + 2].y = ty + font->nh;
        tex_pos[vert_i + 3].x = tx;
        tex_pos[vert_i + 3].y = ty + font->nh;
    }

    shader_program_set_data_float(&prog, 0, text_size * 4, 2, pos);
    shader_program_set_data_float(&prog, 1, text_size * 4, 2, tex_pos);

    shader_program_set_uniform_mat4(&prog, 0, &projection);
    shader_program_set_uniform_vec4(&prog, 1, &color);

    graphics_draw_quads(text_size * 4);
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
