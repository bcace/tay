#include "main.h"
#include "entorama.h"
#include "graphics.h"
#include "shaders.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

#define MAX_QUADS   32
#define MAX_LABEL   256
#define MAX_BUTTONS 32
#define MAX_TOOLTIP 512


extern int paused = 1;

static Program prog;
static Program text_prog;

typedef struct {
    vec2 p1, p2, p3, p4;
} VertQuad;

typedef struct {
    vec4 c1, c2, c3, c4;
} ColorQuad;

typedef struct {
    char label[MAX_LABEL];
    int label_x, label_y;
    vec2 min, max;
} Button;

static int window_w;
static int window_h;
static int toolbar_h;
static int statusbar_h;
static int sidebar_w;

static VertQuad quad_verts[MAX_QUADS];
static ColorQuad quad_colors[MAX_QUADS];
static int quads_count;

static FontTextBuffer text_buffer;

static Button buttons[MAX_BUTTONS];
static int buttons_count;
static Button sidebar_buttons[MAX_BUTTONS];
static int sidebar_buttons_count;
static Button *run_button;
static Button *hovered_button;
static Button *pressed_button;
static Button *selected_sidebar_button;

static char tooltip[MAX_TOOLTIP];

void widgets_init() {
    shader_program_init(&prog, flat_vert, "flat.vert", "", flat_frag, "flat.frag", "");
    shader_program_define_in_float(&prog, 2);            /* vertex position */
    shader_program_define_in_float(&prog, 4);            /* vertex color */
    shader_program_define_uniform(&prog, "projection");

    shader_program_init(&text_prog, text_vert, "text.vert", "", text_frag, "text.frag", "");
    shader_program_define_in_float(&text_prog, 2);            /* vertex position */
    shader_program_define_in_float(&text_prog, 2);            /* vertex texture position */
    shader_program_define_in_float(&text_prog, 4);            /* vertex color */
    shader_program_define_uniform(&text_prog, "projection");

    buttons_count = 0;

    run_button = buttons + buttons_count++;
    strcpy_s(run_button->label, MAX_LABEL, "Run");

    tooltip[0] = '\0';
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

static void _update_sidebar_buttons_geometry() {
    const int SIDEBAR_BUTTON_H = 28;

    for (int button_i = 0; button_i < sidebar_buttons_count; ++button_i) {
        Button *button = sidebar_buttons + button_i;
        button->min = (vec2){0.0f, (float)(window_h - toolbar_h - SIDEBAR_BUTTON_H * (button_i + 1))};
        button->max = (vec2){(float)sidebar_w, (float)(window_h - toolbar_h - SIDEBAR_BUTTON_H * button_i)};
        button->label_x = (int)button->min.x + 10;
        button->label_y = (int)((button->min.y + button->max.y - font_height(ENTORAMA_FONT_MEDIUM)) * 0.5f);
    }
}

void widgets_update_model_specific(struct EntoramaModel *model) {
    sidebar_buttons_count = 0;

    for (unsigned group_i = 0; group_i < model->groups_count; ++group_i) {
        EntoramaGroup *group = model->groups + group_i;

        Button *button = sidebar_buttons + sidebar_buttons_count++;
        strcpy_s(button->label, MAX_LABEL, group->name);
    }

    _update_sidebar_buttons_geometry();
}

void widgets_update(int window_w_in, int window_h_in, int toolbar_h_in, int statusbar_h_in, int sidebar_w_in) {
    window_w = window_w_in;
    window_h = window_h_in;
    toolbar_h = toolbar_h_in;
    statusbar_h = statusbar_h_in;
    sidebar_w = sidebar_w_in;

    /* toolbar buttons */
    {
        const float TOOL_BUTTON_W = 60.0f;

        unsigned text_w = font_text_width(ENTORAMA_FONT_MEDIUM, run_button->label);
        run_button->label_x = (int)((window_w - text_w) * 0.5f);
        run_button->label_y = window_h - (int)((toolbar_h + font_height(ENTORAMA_FONT_MEDIUM)) * 0.5f);
        run_button->min = (vec2){(window_w - TOOL_BUTTON_W) * 0.5f, (float)(window_h - toolbar_h)};
        run_button->max = (vec2){(window_w + TOOL_BUTTON_W) * 0.5f, (float)(window_h)};
    }

    _update_sidebar_buttons_geometry();
}

void widgets_draw(mat4 projection, double ms) {
    graphics_enable_depth_test(0);

    quads_count = 0;

    vec4 fg_color = color_fg();
    vec4 hv_color = fg_color;
    hv_color.w = 0.25f;

    const float shadow_alpha = 0.2f;

    /* quads */
    {
        /* sidebar */
        {
            _init_quad(quad_verts + quads_count, 0.0f, (float)sidebar_w, (float)statusbar_h, (float)(window_h - toolbar_h));
            _init_color(quad_colors + quads_count, color_vd());
            ++quads_count;
        }

        if (selected_sidebar_button) {
            _init_quad(quad_verts + quads_count, selected_sidebar_button->min.x, selected_sidebar_button->max.x, selected_sidebar_button->min.y, selected_sidebar_button->max.y);
            _init_color(quad_colors + quads_count, color_hi());
            ++quads_count;
        }

        /* button highlights */
        if (pressed_button) {
            _init_quad(quad_verts + quads_count, pressed_button->min.x, pressed_button->max.x, pressed_button->min.y, pressed_button->max.y);
            _init_color(quad_colors + quads_count, color_hi());
            ++quads_count;
        }
        else if (hovered_button) {
            _init_quad(quad_verts + quads_count, hovered_button->min.x, hovered_button->max.x, hovered_button->min.y, hovered_button->max.y);
            _init_color(quad_colors + quads_count, hv_color);
            ++quads_count;
        }

        /* shadows */
        {
            _init_quad(quad_verts + quads_count, 0.0f, (float)window_w, (float)(window_h - toolbar_h - 5), (float)(window_h - toolbar_h));
            _init_shadow(quad_colors + quads_count, 0.0f, 0.0f, shadow_alpha, shadow_alpha);
            ++quads_count;

            _init_quad(quad_verts + quads_count, 0.0f, (float)window_w, (float)statusbar_h, (float)(statusbar_h + 5));
            _init_shadow(quad_colors + quads_count, shadow_alpha, shadow_alpha, 0.0f, 0.0f);
            ++quads_count;
        }

        shader_program_use(&prog);
        shader_program_set_uniform_mat4(&prog, 0, &projection);

        if (quads_count) {
            shader_program_set_data_float(&prog, 0, quads_count * 4, 2, quad_verts);
            shader_program_set_data_float(&prog, 1, quads_count * 4, 4, quad_colors);
            graphics_draw_quads(quads_count * 4);
        }
    }

    /* text */
    {
        shader_program_use(&text_prog);

        font_use_medium();

        /* buttons */
        for (int button_i = 0; button_i < buttons_count; ++button_i) {
            Button *b = buttons + button_i;
            font_draw_text(b->label, b->label_x, b->label_y, fg_color, &text_buffer);
        }

        /* sidebar buttons */
        for (int button_i = 0; button_i < sidebar_buttons_count; ++button_i) {
            Button *b = sidebar_buttons + button_i;
            font_draw_text(b->label, b->label_x, b->label_y, fg_color, &text_buffer);
        }

        /* simulation speed */
        {
            char buffer[50];
            sprintf_s(buffer, 50, "%.1f ms", ms);
            font_draw_text(buffer,
                           window_w - font_text_width(ENTORAMA_FONT_MEDIUM, buffer) - 10,
                           (int)((statusbar_h - font_height(ENTORAMA_FONT_MEDIUM)) * 0.5f),
                           fg_color, &text_buffer);
        }

        /* tooltip */
        {
            font_draw_text(tooltip,
                           (int)((window_w - font_text_width(ENTORAMA_FONT_MEDIUM, tooltip)) * 0.5f),
                           (int)((statusbar_h - font_height(ENTORAMA_FONT_MEDIUM)) * 0.5f),
                           fg_color, &text_buffer);
        }

        shader_program_set_data_float(&text_prog, 0, text_buffer.count * 4, 2, text_buffer.pos);
        shader_program_set_data_float(&text_prog, 1, text_buffer.count * 4, 2, text_buffer.tex);
        shader_program_set_data_float(&text_prog, 2, text_buffer.count * 4, 4, text_buffer.col);
        shader_program_set_uniform_mat4(&text_prog, 0, &projection);
        graphics_draw_quads(text_buffer.count * 4);
    }

    font_text_buffer_clear(&text_buffer);
}

void widgets_mouse_move(int button_l, int button_r, float x, float y) {
    hovered_button = 0;
    tooltip[0] = '\0';

    if (x > run_button->min.x && x < run_button->max.x && y > run_button->min.y && y < run_button->max.y) {
        hovered_button = run_button;
        strcpy_s(tooltip, MAX_TOOLTIP, "Run/pause simulation");
        return;
    }

    for (int button_i = 0; button_i < sidebar_buttons_count; ++button_i) {
        Button *button = sidebar_buttons + button_i;

        if (x > button->min.x && x < button->max.x && y > button->min.y && y < button->max.y) {
            hovered_button = button;
            strcpy_s(tooltip, MAX_TOOLTIP, button->label);
            return;
        }
    }
}

static int _is_sidebar_button(Button *button) {
    return button >= sidebar_buttons && button < sidebar_buttons + sidebar_buttons_count;
}

void widgets_mouse_button(int button, int action) {
    if (button == 0) {
        if (action == 0)
            pressed_button = hovered_button;
        else {
            if (pressed_button == hovered_button) { /* for a button click press button must be the same as the release button */
                selected_sidebar_button = 0;
                if (pressed_button == run_button) { /* temporary run button action */
                    paused = !paused;
                }
                else if (_is_sidebar_button(pressed_button)) {
                    selected_sidebar_button = pressed_button;
                }
            }
            pressed_button = 0;
        }
    }
}
