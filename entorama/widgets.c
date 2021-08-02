#include "main.h"
#include "entorama.h"
#include "widgets.h"
#include "graphics.h"
#include "shaders.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

#define EM_MAX_LAYERS                   8
#define EM_BUTTON_DEFAULT_LABEL_OFFSET  10.0f


int mouse_l = 0;
int mouse_r = 0;
float mouse_x;
float mouse_y;
int redraw = 0;

static redraw_next_frame = 0; /* when a widget wants to redraw the entire UI as part of its response, the redraw has to happen next frame */

static Program prog;
static Program text_prog;

typedef struct {
    TexQuadBuffer text_buffer;
    QuadBuffer quad_buffer;
    int scissor_enabled;
    int scissor_x, scissor_y;
    int scissor_w, scissor_h;
} Layer;

static Layer layers[EM_MAX_LAYERS];
static Layer *selected_layer = layers;

static unsigned available_widget_id = 0;
static unsigned pressed_widget_id = 0;
static unsigned hovered_widget_id = 0;
static unsigned previous_hovered_widget_id = 0;

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

    if (redraw_next_frame) {
        redraw_next_frame = 0;
        redraw = 1;
    }

    previous_hovered_widget_id = hovered_widget_id;
    hovered_widget_id = 0;
    available_widget_id = 0;

    for (unsigned layer_i = 0; layer_i < EM_MAX_LAYERS; ++layer_i) {
        Layer *layer = layers + layer_i;
        quad_buffer_clear(&layer->quad_buffer);
        tex_quad_buffer_clear(&layer->text_buffer);
    }
}

void em_widgets_end() {
    if (!mouse_l)
        pressed_widget_id = 0;

    if (hovered_widget_id != previous_hovered_widget_id)
        redraw = 1;
}

void em_widgets_draw(struct mat4 projection) {

    for (unsigned layer_i = 0; layer_i < EM_MAX_LAYERS; ++layer_i) {
        Layer *layer = layers + layer_i;

        if (layer->scissor_enabled)
            graphics_enable_scissor(layer->scissor_x, layer->scissor_y, layer->scissor_w, layer->scissor_h);

        if (layer->quad_buffer.count) {
            shader_program_use(&prog);
            shader_program_set_uniform_mat4(&prog, 0, &projection);
            shader_program_set_data_float(&prog, 0, layer->quad_buffer.count * 4, 2, layer->quad_buffer.pos);
            shader_program_set_data_float(&prog, 1, layer->quad_buffer.count * 4, 4, layer->quad_buffer.col);
            graphics_draw_quads(layer->quad_buffer.count * 4);
        }

        if (layer->text_buffer.count) {
            font_use_medium();
            shader_program_use(&text_prog);
            shader_program_set_data_float(&text_prog, 0, layer->text_buffer.count * 4, 2, layer->text_buffer.pos);
            shader_program_set_data_float(&text_prog, 1, layer->text_buffer.count * 4, 2, layer->text_buffer.tex);
            shader_program_set_data_float(&text_prog, 2, layer->text_buffer.count * 4, 4, layer->text_buffer.col);
            shader_program_set_uniform_mat4(&text_prog, 0, &projection);
            graphics_draw_quads(layer->text_buffer.count * 4);
        }

        if (layer->scissor_enabled)
            graphics_disable_scissor();
    }
}

static EmResponse _get_response(unsigned id, float min_x, float min_y, float max_x, float max_y, EmButtonFlags flags) {
    EmResponse response = EM_RESPONSE_NONE;

    int disabled = flags & EM_BUTTON_FLAGS_DISABLED;

    if (mouse_x >= min_x && mouse_x <= max_x && mouse_y >= min_y && mouse_y <= max_y) {
        if (mouse_l) {
            if (pressed_widget_id == 0 && !disabled) {
                pressed_widget_id = id;
                redraw_next_frame = 1;
            }
        }
        else {
            if (pressed_widget_id == id) {
                response = EM_RESPONSE_CLICKED;
                pressed_widget_id = 0;
                redraw_next_frame = 1;
            }
            else if (!disabled)
                response = EM_RESPONSE_HOVERED;
            hovered_widget_id = id;
        }
    }
    else if (pressed_widget_id == id) {
        response = EM_RESPONSE_PRESSED;
        redraw_next_frame = 1;
    }

    return response;
}

EmResponse em_button(char *label, float min_x, float min_y, float max_x, float max_y, EmButtonFlags flags) {
    unsigned id = ++available_widget_id;

    unsigned label_w = font_text_width(ENTORAMA_FONT_MEDIUM, label);
    unsigned label_h = font_height(ENTORAMA_FONT_MEDIUM);
    int label_x = (int)(min_x + button_label_offset); // (int)((min_x + max_x - label_w) * 0.5f);
    int label_y = (int)((min_y + max_y - label_h) * 0.5f);

    if (flags & EM_BUTTON_FLAGS_DISABLED)
        font_draw_text(ENTORAMA_FONT_MEDIUM, label, label_x, label_y, color_fg_disabled(), &selected_layer->text_buffer);
    else
        font_draw_text(ENTORAMA_FONT_MEDIUM, label, label_x, label_y, color_fg(), &selected_layer->text_buffer);

    EmResponse response = _get_response(id, min_x, min_y, max_x, max_y, flags);

    vec2 *quad_pos = 0;
    vec4 *quad_col = 0;

    if (pressed_widget_id == id || flags & EM_BUTTON_FLAGS_PRESSED) {
        quad_buffer_add(&selected_layer->quad_buffer, 1, &quad_pos, &quad_col);
        _init_quad(quad_pos, min_x, max_x, min_y, max_y);
        _init_color(quad_col, color_hi());
    }

    if (hovered_widget_id == id) {
        quad_buffer_add(&selected_layer->quad_buffer, 1, &quad_pos, &quad_col);
        _init_quad(quad_pos, min_x, max_x, min_y, max_y);
        _init_color(quad_col, color_fg_hover());
    }

    return response;
}

EmResponse em_area(char *label, float min_x, float min_y, float max_x, float max_y) {
    unsigned id = ++available_widget_id;

    return _get_response(id, min_x, min_y, max_x, max_y, EM_BUTTON_FLAGS_NONE);
}

EmResponse em_label(char *label, float min_x, float min_y, float max_x, float max_y, EmButtonFlags flags) {
    unsigned id = ++available_widget_id;

    unsigned label_w = font_text_width(ENTORAMA_FONT_MEDIUM, label);
    unsigned label_h = font_height(ENTORAMA_FONT_MEDIUM);
    int label_x = (flags & EM_WIDGET_FLAGS_LEFT) ? (int)(min_x + button_label_offset) : (int)((min_x + max_x - label_w) * 0.5f);
    int label_y = (int)((min_y + max_y - label_h) * 0.5f);

    if (flags & EM_BUTTON_FLAGS_DISABLED)
        font_draw_text(ENTORAMA_FONT_MEDIUM, label, label_x, label_y, color_fg_disabled(), &selected_layer->text_buffer);
    else
        font_draw_text(ENTORAMA_FONT_MEDIUM, label, label_x, label_y, color_fg(), &selected_layer->text_buffer);

    return EM_RESPONSE_NONE;
}

void em_quad(float min_x, float min_y, float max_x, float max_y, vec4 color) {
    vec2 *quad_pos = 0;
    vec4 *quad_col = 0;

    quad_buffer_add(&selected_layer->quad_buffer, 1, &quad_pos, &quad_col);
    _init_quad(quad_pos, min_x, max_x, min_y, max_y);
    _init_color(quad_col, color);
}

void em_set_button_label_offset(float offset) {
    button_label_offset = offset;
}

void em_reset_button_label_offset() {
    button_label_offset = EM_BUTTON_DEFAULT_LABEL_OFFSET;
}

void em_select_layer(unsigned layer_index) {
    selected_layer = layers + (layer_index % EM_MAX_LAYERS);
}

void em_set_layer_scissor(float min_x, float min_y, float max_x, float max_y) {
    selected_layer->scissor_enabled = 1;
    selected_layer->scissor_x = (int)min_x;
    selected_layer->scissor_y = (int)min_y;
    selected_layer->scissor_w = (int)(max_x - min_x);
    selected_layer->scissor_h = (int)(max_y - min_y);
}

int em_widget_pressed() {
    return pressed_widget_id != 0;
}
