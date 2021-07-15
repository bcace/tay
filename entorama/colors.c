#include "main.h"
#include "graphics.h"


typedef struct {
    vec4 vd;
    vec4 bg;
    vec4 fg;
    vec4 hi;
    vec4 palette[4];
} Theme;

Theme dark, light;
Theme *selected;

void color_init() {
    dark = (Theme){
        // .bg = (vec4){0.15f, 0.15f, 0.15f, 1.0f},
        .vd = (vec4){0.0664f * 0.7f, 0.18359f * 0.7f, 0.2539f * 0.7f, 1.0f},
        .bg = (vec4){0.0664f, 0.18359f, 0.2539f, 1.0f},
        .fg = (vec4){0.8f, 0.8f, 0.8f, 1.0f},
        .hi = (vec4){0.8f, 0.0f, 0.0f, 1.0f},
        .palette = {
            (vec4){0.019531f, 0.51953125f, 0.52734375f, 1.0f},
            (vec4){0.30859375f, 0.71875f, 0.6171875f, 1.0f},
            (vec4){0.9453125f, 0.6914f, 0.203125f, 1.0f},
            (vec4){0.92578125f, 0.328125f, 0.23046875f, 1.0f},
        },
    };

    light = (Theme){
        .vd = (vec4){0.9f, 0.9f, 0.9f, 1.0f},
        .bg = (vec4){1.0f, 1.0f, 1.0f, 1.0f},
        .fg = (vec4){0.1f, 0.1f, 0.1f, 1.0f},
        .hi = (vec4){0.8f, 0.0f, 0.0f, 1.0f},
        .palette = {
            (vec4){0.019531f, 0.51953125f, 0.52734375f, 1.0f},
            (vec4){0.30859375f, 0.71875f, 0.6171875f, 1.0f},
            (vec4){0.9453125f, 0.6914f, 0.203125f, 1.0f},
            (vec4){0.92578125f, 0.328125f, 0.23046875f, 1.0f},
        },
    };

    selected = &light;
    // selected = &dark;
}

vec4 color_vd() {
    return selected->vd;
}

vec4 color_bg() {
    return selected->bg;
}

vec4 color_fg() {
    return selected->fg;
}

vec4 color_hi() {
    return selected->hi;
}

vec4 color_palette(int index) {
    return selected->palette[index % 4];
}
