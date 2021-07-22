#include "main.h"
#include "entorama.h"
#include "widgets.h"
#include "graphics.h"
#include "shaders.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

#define EM_BUTTON_DEFAULT_LABEL_OFFSET 10.0f


int mouse_l = 0;
int mouse_r = 0;
float mouse_x;
float mouse_y;

static Program prog;
static Program text_prog;

static TexQuadBuffer text_buffer;
static QuadBuffer quad_buffer;

static unsigned long long pressed_widget_id = 0;
static unsigned long long hovered_widget_id = 0;

static float button_label_offset = EM_BUTTON_DEFAULT_LABEL_OFFSET;

void em_widgets_init() {
    shader_program_init(&prog, flat_vert, "flat.vert", "", flat_frag, "flat.frag", "");
    shader_program_define_in_float(&prog, 2);            /* vertex position */
    shader_program_define_in_float(&prog, 4);            /* vertex color */
    shader_program_define_uniform(&prog, "projection");

    shader_program_init(&text_prog, text_vert, "text.vert", "", text_frag, "text.frag", "");
    shader_program_define_in_float(&text_prog, 2);            /* vertex position */
    shader_program_define_in_float(&text_prog, 2);            /* vertex texture position */
    shader_program_define_in_float(&text_prog, 4);            /* vertex color */
    shader_program_define_uniform(&text_prog, "projection");
}

static void _init_quad(vec2 *quad, float min_x, float max_x, float min_y, float max_y) {
    quad[0].x = min_x;
    quad[0].y = min_y;
    quad[1].x = max_x;
    quad[1].y = min_y;
    quad[2].x = max_x;
    quad[2].y = max_y;
    quad[3].x = min_x;
    quad[3].y = max_y;
}

static void _init_color(vec4 *quad, vec4 color) {
    quad[0] = color;
    quad[1] = color;
    quad[2] = color;
    quad[3] = color;
}

void em_widgets_begin() {
    graphics_enable_depth_test(0);
    font_use_medium();
    hovered_widget_id = 0;
}

void em_widgets_end(mat4 projection) {
    if (quad_buffer.count) {
        shader_program_use(&prog);
        shader_program_set_uniform_mat4(&prog, 0, &projection);
        shader_program_set_data_float(&prog, 0, quad_buffer.count * 4, 2, quad_buffer.pos);
        shader_program_set_data_float(&prog, 1, quad_buffer.count * 4, 4, quad_buffer.col);
        graphics_draw_quads(quad_buffer.count * 4);
        quad_buffer_clear(&quad_buffer);
    }

    if (text_buffer.count) {
        shader_program_use(&text_prog);
        shader_program_set_data_float(&text_prog, 0, text_buffer.count * 4, 2, text_buffer.pos);
        shader_program_set_data_float(&text_prog, 1, text_buffer.count * 4, 2, text_buffer.tex);
        shader_program_set_data_float(&text_prog, 2, text_buffer.count * 4, 4, text_buffer.col);
        shader_program_set_uniform_mat4(&text_prog, 0, &projection);
        graphics_draw_quads(text_buffer.count * 4);
        tex_quad_buffer_clear(&text_buffer);
    }

    if (!mouse_l)
        pressed_widget_id = 0;
}

static EmResponse _get_response(unsigned long long id, float min_x, float min_y, float max_x, float max_y, EmButtonFlags flags) {
    EmResponse response = EM_RESPONSE_NONE;

    if (mouse_x >= min_x && mouse_x <= max_x && mouse_y >= min_y && mouse_y <= max_y) {
        if (mouse_l) {
            if (pressed_widget_id == 0) {
                response = EM_RESPONSE_PRESSED;
                pressed_widget_id = id;
            }
        }
        else {
            if (pressed_widget_id == id) {
                response = EM_RESPONSE_CLICKED;
                pressed_widget_id = 0;
            }
            else
                response = EM_RESPONSE_HOVERED;
            hovered_widget_id = id;
        }
    }
    else if (pressed_widget_id == id)
        response = EM_RESPONSE_PRESSED;

    return response;
}

EmResponse em_button(char *label, float min_x, float min_y, float max_x, float max_y, EmButtonFlags flags) {
    unsigned label_w = font_text_width(ENTORAMA_FONT_MEDIUM, label);
    unsigned label_h = font_height(ENTORAMA_FONT_MEDIUM);
    int label_x = (int)(min_x + button_label_offset); // (int)((min_x + max_x - label_w) * 0.5f);
    int label_y = (int)((min_y + max_y - label_h) * 0.5f);

    font_draw_text(label, label_x, label_y, color_fg(), &text_buffer);

    EmResponse response = _get_response((unsigned long long)label, min_x, min_y, max_x, max_y, flags);

    vec2 *quad_pos = 0;
    vec4 *quad_col = 0;

    if (pressed_widget_id == (unsigned long long)label || flags & EM_BUTTON_FLAGS_PRESSED) {
        quad_buffer_add(&quad_buffer, 1, &quad_pos, &quad_col);
        _init_quad(quad_pos, min_x, max_x, min_y, max_y);
        _init_color(quad_col, color_hi());
    }

    if (hovered_widget_id == (unsigned long long)label) {
        quad_buffer_add(&quad_buffer, 1, &quad_pos, &quad_col);
        _init_quad(quad_pos, min_x, max_x, min_y, max_y);
        _init_color(quad_col, color_fg_hover());
    }

    return response;
}

EmResponse em_button_described(char *label, char *description, float min_x, float min_y, float max_x, float max_y, EmButtonFlags flags) {
    unsigned label_w = font_text_width(ENTORAMA_FONT_MEDIUM, label);
    unsigned label_h = font_height(ENTORAMA_FONT_MEDIUM);
    int label_x = (int)(min_x + button_label_offset); // (int)((min_x + max_x - label_w) * 0.5f);
    int label_y = (int)(max_y - label_h - 9);

    font_draw_text(label, label_x, label_y, color_fg(), &text_buffer);

    unsigned desc_w = font_text_width(ENTORAMA_FONT_MEDIUM, description);
    unsigned desc_h = font_height(ENTORAMA_FONT_MEDIUM);
    int desc_x = (int)(min_x + button_label_offset);
    int desc_y = (int)(min_y + 9);

    font_draw_text(description, desc_x, desc_y, color_fg_disabled(), &text_buffer);

    EmResponse response = _get_response((unsigned long long)label, min_x, min_y, max_x, max_y, flags);

    vec2 *quad_pos = 0;
    vec4 *quad_col = 0;

    if (pressed_widget_id == (unsigned long long)label || flags & EM_BUTTON_FLAGS_PRESSED) {
        quad_buffer_add(&quad_buffer, 1, &quad_pos, &quad_col);
        _init_quad(quad_pos, min_x, max_x, min_y, max_y);
        _init_color(quad_col, color_hi());
    }

    if (hovered_widget_id == (unsigned long long)label) {
        quad_buffer_add(&quad_buffer, 1, &quad_pos, &quad_col);
        _init_quad(quad_pos, min_x, max_x, min_y, max_y);
        _init_color(quad_col, color_fg_hover());
    }

    return response;
}

EmResponse em_area(char *label, float min_x, float min_y, float max_x, float max_y) {
    return _get_response((unsigned long long)label, min_x, min_y, max_x, max_y, EM_BUTTON_FLAGS_NONE);
}

EmResponse em_label(char *label, float min_x, float min_y, float max_x, float max_y) {
    unsigned label_w = font_text_width(ENTORAMA_FONT_MEDIUM, label);
    unsigned label_h = font_height(ENTORAMA_FONT_MEDIUM);
    int label_x = (int)((min_x + max_x - label_w) * 0.5f);
    int label_y = (int)((min_y + max_y - label_h) * 0.5f);

    font_draw_text(label, label_x, label_y, color_fg(), &text_buffer);

    return EM_RESPONSE_NONE;
}

void em_quad(float min_x, float min_y, float max_x, float max_y, vec4 color) {
    vec2 *quad_pos = 0;
    vec4 *quad_col = 0;

    quad_buffer_add(&quad_buffer, 1, &quad_pos, &quad_col);
    _init_quad(quad_pos, min_x, max_x, min_y, max_y);
    _init_color(quad_col, color);
}

void em_set_button_label_offset(float offset) {
    button_label_offset = offset;
}

void em_reset_button_label_offset() {
    button_label_offset = EM_BUTTON_DEFAULT_LABEL_OFFSET;
}

int em_widget_pressed() {
    return pressed_widget_id != 0;
}
