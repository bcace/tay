#include "main.h"
#include "entorama.h"
#include "graphics.h"


typedef enum {
    CAMERA_MODELING,
    CAMERA_FLOATING,
} CameraType;

typedef struct {
    CameraType type;
    union {
        struct {
            float pan_x;
            float pan_y;
            float rot_x;
            float rot_y;
            float zoom;
        };
        struct {
            float fov;
            float near;
            float far;
            vec3 pos;
            vec3 fwd;
            vec3 up;
        };
    };
} Camera;

static Camera camera;

void drawing_init() {
    camera.type = CAMERA_MODELING;

    camera.pan_x = 0.0f;
    camera.pan_y = 0.0f;
    camera.rot_x = -1.0f;
    camera.rot_y = 0.0f;
    camera.zoom = 0.2f;

    // camera.fov = 1.2f;
    // camera.near = 0.1f;
    // camera.pos.x = model_info.origin_x;
    // camera.pos.y = model_info.origin_y;
    // camera.pos.z = model_info.origin_z + model_info.radius * 4.0f;
    // camera.fwd.x = 0.0f;
    // camera.fwd.y = 0.0f;
    // camera.fwd.z = -1.0;
    // camera.up.x = 0.0f;
    // camera.up.y = 1.0f;
    // camera.up.z = 0.0f;
    // camera.far = model_info.radius * 6.0f;
}

void drawing_mouse_scroll(double y) {
    if (camera.type == CAMERA_MODELING) {
        if (y > 0.0) {
            camera.zoom *= 0.95f;
            camera.pan_x /= 0.95f;
            camera.pan_y /= 0.95f;
        }
        else if (y < 0.0) {
            camera.zoom /= 0.95f;
            camera.pan_x *= 0.95f;
            camera.pan_y *= 0.95f;
        }
    }
}

void drawing_mouse_move(int button_l, int button_r, float dx, float dy) {
    if (camera.type == CAMERA_MODELING) {
        if (button_l) {
            camera.rot_x -= dy * 0.001f;
            camera.rot_y += dx * 0.001f;
        }
        else if (button_r) {
            camera.pan_x -= dx;
            camera.pan_y -= dy;
        }
    }
}

void drawing_camera_setup(EntoramaModelInfo *info, int window_w, int window_h, mat4 *projection, mat4 *modelview) {
    mat4_set_identity(modelview);
    if (camera.type == CAMERA_MODELING) {
        graphics_frustum(projection,
            0.00001f * camera.zoom * (camera.pan_x - window_w * 0.5f),
            0.00001f * camera.zoom * (camera.pan_x + window_w * 0.5f),
            0.00001f * camera.zoom * (camera.pan_y - window_h * 0.5f),
            0.00001f * camera.zoom * (camera.pan_y + window_h * 0.5f),
            0.001f, 200.0f
        );
        mat4_translate(modelview, 0.0f, 0.0f, -100.0f);
        mat4_rotate(modelview, camera.rot_x, 1.0f, 0.0f, 0.0f);
        mat4_rotate(modelview, camera.rot_y, 0.0f, 0.0f, 1.0f);
        mat4_scale(modelview, 50.0f / info->radius);
        mat4_translate(modelview, -info->origin_x, -info->origin_y, -info->origin_z);
    }
    else {
        mat4 perspective, lookat;
        graphics_perspective(&perspective, camera.fov, (float)window_w / (float)window_h, camera.near, camera.far);
        graphics_lookat(&lookat, camera.pos, camera.fwd, camera.up);
        mat4_multiply(projection, &perspective, &lookat);
        mat4_set_identity(modelview);
    }
}
