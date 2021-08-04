#ifndef font_rasters_h
#define font_rasters_h


typedef struct FontRaster {
    unsigned w;
    unsigned h;
    unsigned iw;
    unsigned ih;
    unsigned code_size;
    unsigned data_size;
    unsigned char *data;
} FontRaster;

FontRaster *font_raster_get();
void font_raster_clear();

void font_raster_create_inconsolata8x16();
void font_raster_create_iosevka8x16();
void font_raster_create_iosevka7x16();

#endif
