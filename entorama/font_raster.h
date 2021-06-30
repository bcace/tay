#ifndef font_rasters_h
#define font_rasters_h


typedef struct FontRaster {
    int w;
    int h;
    int iw;
    int ih;
    int code_size;
    int data_size;
    unsigned char *data;
} FontRaster;

FontRaster *font_raster_get();
void font_raster_clear();

void font_raster_create_inconsolata13();

#endif
