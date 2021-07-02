#ifndef font_h
#define font_h


typedef struct Font {
    unsigned int id;
    int w;
    int h;
    float nw;
    float nh;
} Font;

void font_init();
void font_use_medium();

#endif
