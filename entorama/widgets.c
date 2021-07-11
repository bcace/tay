#include "main.h"
#include "graphics.h"
#include "shaders.h"
#include "font.h"
#include <stdio.h>

#define MAX_PANES 8


static Program prog;

typedef struct {
    vec2 min, max;
} Rect;

static Rect panes[MAX_PANES];
static int panes_count;
static int window_w;
static int window_h;
static int toolbar_h;

void widgets_init() {
    shader_program_init(&prog, flat_vert, "flat.vert", "", flat_frag, "flat.frag", "");
    shader_program_define_in_float(&prog, 2);            /* vertex position */
    shader_program_define_uniform(&prog, "projection");
    shader_program_define_uniform(&prog, "uniform_color");
}

void widgets_update(int window_w_in, int window_h_in, int toolbar_h_in) {
    panes_count = 0;
    window_w = window_w_in;
    window_h = window_h_in;
    toolbar_h = toolbar_h_in;

    {
        Rect *pane = panes + panes_count++;
        pane->min.x = 0.0f;
        pane->min.y = (float)(window_h - toolbar_h);
        pane->max.x = (float)window_w;
        pane->max.y = (float)window_h;
    }
}

void widgets_draw(mat4 projection, double ms) {
    graphics_enable_depth_test(0);

    /* panes */
    {
        shader_program_use(&prog);

        static vec2 pos[MAX_PANES * 4];

        for (int pane_i = 0; pane_i < panes_count; ++pane_i) {
            Rect *pane = panes + pane_i;
            int pos_i = pane_i * 4;
            pos[pos_i + 0] = pane->min;
            pos[pos_i + 1].x = pane->max.x;
            pos[pos_i + 1].y = pane->min.y;
            pos[pos_i + 2] = pane->max;
            pos[pos_i + 3].x = pane->min.x;
            pos[pos_i + 3].y = pane->max.y;
        }

        shader_program_set_data_float(&prog, 0, panes_count * 4, 2, pos);

        shader_program_set_uniform_mat4(&prog, 0, &projection);
        vec4 bkbg_color = color_bkbg();
        shader_program_set_uniform_vec4(&prog, 1, &bkbg_color);

        graphics_draw_quads(panes_count * 4);
    }

    /* simulation speed */
    {
        font_use_medium();
        char buffer[50];
        sprintf_s(buffer, 50, "%.1f ms", ms);
        font_draw_text(buffer, window_w - font_text_length(buffer) - 10, window_h - (int)((toolbar_h + font_height()) * 0.5f), projection, color_fg());
    }
}
