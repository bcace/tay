#include "main.h"
#include "graphics.h"


vec4 color_bkbg() {
    return (vec4){0.0664f * 0.7f, 0.18359f * 0.7f, 0.2539f * 0.7f, 1.0f};
}

vec4 color_bg() {
    return (vec4){0.0664f, 0.18359f, 0.2539f, 1.0f};
}

vec4 color_fg() {
    return (vec4){0.30859375f, 0.71875f, 0.6171875f, 1.0f};
}

vec4 color_palette(int index) {
    switch (index) {
        case 0: return (vec4){0.019531f, 0.51953125f, 0.52734375f, 1.0f};
        case 1: return (vec4){0.30859375f, 0.71875f, 0.6171875f, 1.0f};
        case 2: return (vec4){0.9453125f, 0.6914f, 0.203125f, 1.0f};
        default: return (vec4){0.92578125f, 0.328125f, 0.23046875f, 1.0f};
    }
}
