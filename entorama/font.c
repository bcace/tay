#include "font.h"
#include "font_raster.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <stdlib.h>


typedef struct {
    unsigned int id;
    int w;
    int h;
    float nw;
    float nh;
} Font;

Font inconsolata13;

static void _create_texture(Font *font, FontRaster *raster) {
    font->w = raster->w;
    font->h = raster->h;
    font->nw = raster->w / (float)raster->iw;
    font->nh = raster->h / (float)raster->ih;

    glGenTextures(1, &font->id);
    glBindTexture(GL_TEXTURE_2D, font->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, raster->iw, raster->ih, 0, GL_RGBA, GL_UNSIGNED_BYTE, raster->data);
}

void font_init() {
    FontRaster *raster = font_raster_get();

    font_raster_create_inconsolata13();
    _create_texture(&inconsolata13, raster);

    font_raster_clear();
}
