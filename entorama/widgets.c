#include "main.h"
#include "graphics.h"
#include "shaders.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

#define MAX_QUADS   32
#define MAX_LABEL   128
#define MAX_BUTTONS 32


static Program prog;

typedef struct {
    vec2 p1, p2, p3, p4;
} VertQuad;

typedef struct {
    vec4 c1, c2, c3, c4;
} ColorQuad;

typedef struct {
    char label[MAX_LABEL];
    int label_x, label_y;
} Button;

static VertQuad quad_verts[MAX_QUADS];
static ColorQuad quad_colors[MAX_QUADS];
static Button buttons[MAX_BUTTONS];
static int quads_count;
static int window_w;
static int window_h;
static int toolbar_h;
static int buttons_count;

void widgets_init() {
    shader_program_init(&prog, flat_vert, "flat.vert", "", flat_frag, "flat.frag", "");
    shader_program_define_in_float(&prog, 2);            /* vertex position */
    shader_program_define_in_float(&prog, 4);            /* vertex color */
    shader_program_define_uniform(&prog, "projection");

    buttons_count = 0;

    {
        Button *b = buttons + buttons_count++;
        strcpy_s(b->label, MAX_LABEL, "Run");
    }
}

static void _init_quad(VertQuad *quad, float min_x, float max_x, float min_y, float max_y) {
    quad->p1.x = min_x;
    quad->p1.y = min_y;
    quad->p2.x = max_x;
    quad->p2.y = min_y;
    quad->p3.x = max_x;
    quad->p3.y = max_y;
    quad->p4.x = min_x;
    quad->p4.y = max_y;
}

static void _init_shadow(ColorQuad *quad, float a1, float a2, float a3, float a4) {
    quad->c1 = (vec4){0.0f, 0.0f, 0.0f, a1};
    quad->c2 = (vec4){0.0f, 0.0f, 0.0f, a2};
    quad->c3 = (vec4){0.0f, 0.0f, 0.0f, a3};
    quad->c4 = (vec4){0.0f, 0.0f, 0.0f, a4};
}

static void _init_color(ColorQuad *quad, vec4 color) {
    quad->c1 = color;
    quad->c2 = color;
    quad->c3 = color;
    quad->c4 = color;
}

void widgets_update(int window_w_in, int window_h_in, int toolbar_h_in) {
    quads_count = 0;
    window_w = window_w_in;
    window_h = window_h_in;
    toolbar_h = toolbar_h_in;

    /* shadows */
    {
        _init_quad(quad_verts + quads_count, 0.0f, (float)window_w, (float)(window_h - toolbar_h - 8), (float)(window_h - toolbar_h));
        _init_shadow(quad_colors + quads_count, 0.0f, 0.0f, 0.5f, 0.5f);
        ++quads_count;
    }

    /* buttons */
    {
        Button *run_b = buttons + 0;
        unsigned text_w = font_text_width(ENTORAMA_FONT_MEDIUM, run_b->label);
        run_b->label_x = (int)((window_w - text_w) * 0.5f);
        run_b->label_y = window_h - (int)((toolbar_h + font_text_height(ENTORAMA_FONT_MEDIUM, run_b->label)) * 0.5f);
    }
}

void widgets_draw(mat4 projection, double ms) {
    graphics_enable_depth_test(0);

    /* quads */
    {
        shader_program_use(&prog);

        shader_program_set_data_float(&prog, 0, quads_count * 4, 2, quad_verts);
        shader_program_set_data_float(&prog, 1, quads_count * 4, 4, quad_colors);

        shader_program_set_uniform_mat4(&prog, 0, &projection);

        graphics_draw_quads(quads_count * 4);
    }

    font_use_medium();

    /* button labels */
    for (int button_i = 0; button_i < buttons_count; ++button_i) {
        Button *b = buttons + button_i;
        font_draw_text(b->label, b->label_x, b->label_y, projection, color_fg());
    }

    /* simulation speed */
    {
        char buffer[50];
        sprintf_s(buffer, 50, "%.1f ms", ms);
        font_draw_text(buffer, window_w - font_text_width(ENTORAMA_FONT_MEDIUM, buffer) - 10, window_h - (int)((toolbar_h + font_text_height(ENTORAMA_FONT_MEDIUM, buffer)) * 0.5f), projection, color_fg());
    }
}
