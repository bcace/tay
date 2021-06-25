#include <math.h>


extern float PYRAMID_VERTS[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 4.0f,

    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 4.0f,

    1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 4.0f,

    -1.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 4.0f,

    -1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,

    -1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
};

extern int PYRAMID_VERTS_COUNT = 18;

extern float ICOSAHEDRON_VERTS[] = {
    0.5257f, 0.0f, 0.8506f, 0.0f, 0.8506f, 0.5257f, -0.5257f, 0.0f, 0.8506f,
    0.0f, 0.8506f, 0.5257f, -0.8506f, 0.5257f, 0.0f, -0.5257f, 0.0f, 0.8506f,
    0.0f, 0.8506f, 0.5257f, 0.0f, 0.8506f, -0.5257f, -0.8506f, 0.5257f, 0.0f,
    0.8506f, 0.5257f, 0.0f, 0.0f, 0.8506f, -0.5257f, 0.0f, 0.8506f, 0.5257f,
    0.5257f, 0.0f, 0.8506f, 0.8506f, 0.5257f, 0.0f, 0.0f, 0.8506f, 0.5257f,
    0.5257f, 0.0f, 0.8506f, 0.8506f, -0.5257f, 0.0f, 0.8506f, 0.5257f, 0.0f,
    0.8506f, -0.5257f, 0.0f, 0.5257f, 0.0f, -0.8506f, 0.8506f, 0.5257f, 0.0f,
    0.8506f, 0.5257f, 0.0f, 0.5257f, 0.0f, -0.8506f, 0.0f, 0.8506f, -0.5257f,
    0.5257f, 0.0f, -0.8506f, -0.5257f, 0.0f, -0.8506f, 0.0f, 0.8506f, -0.5257f,
    0.5257f, 0.0f, -0.8506f, 0.0f, -0.8506f, -0.5257f, -0.5257f, 0.0f, -0.8506f,
    0.5257f, 0.0f, -0.8506f, 0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, -0.5257f,
    0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, 0.5257f, 0.0f, -0.8506f, -0.5257f,
    0.0f, -0.8506f, 0.5257f, -0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, -0.5257f,
    0.0f, -0.8506f, 0.5257f, -0.5257f, 0.0f, 0.8506f, -0.8506f, -0.5257f, 0.0f,
    0.0f, -0.8506f, 0.5257f, 0.5257f, 0.0f, 0.8506f, -0.5257f, 0.0f, 0.8506f,
    0.8506f, -0.5257f, 0.0f, 0.5257f, 0.0f, 0.8506f, 0.0f, -0.8506f, 0.5257f,
    -0.8506f, -0.5257f, 0.0f, -0.5257f, 0.0f, 0.8506f, -0.8506f, 0.5257f, 0.0f,
    -0.5257f, 0.0f, -0.8506f, -0.8506f, -0.5257f, 0.0f, -0.8506f, 0.5257f, 0.0f,
    0.0f, 0.8506f, -0.5257f, -0.5257f, 0.0f, -0.8506f, -0.8506f, 0.5257f, 0.0f,
    -0.8506f, -0.5257f, 0.0f, -0.5257f, 0.0f, -0.8506f, 0.0f, -0.8506f, -0.5257f,
};

extern int ICOSAHEDRON_VERTS_COUNT = 180;

extern float CUBE_VERTS[] = {
    -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f,
    0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,

    0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f,
    0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f,

    0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,
    -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,

    -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

    -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f,

    -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
    -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
};

extern int CUBE_VERTS_COUNT = 36;

int icosahedron_verts_count(unsigned subdivs) {
    return 20 * (1 << (subdivs * 2)) * 3;
}

static float *_icosahedron_subdivide(float *t, unsigned level, float *vert) {
    if (level == 0) {
        *(vert++) = t[0];
        *(vert++) = t[1];
        *(vert++) = t[2];
        *(vert++) = t[3];
        *(vert++) = t[4];
        *(vert++) = t[5];
        *(vert++) = t[6];
        *(vert++) = t[7];
        *(vert++) = t[8];
    }
    else {
        float v[9];

        v[0] = (t[0] + t[6]) * 0.5f;
        v[1] = (t[1] + t[7]) * 0.5f;
        v[2] = (t[2] + t[8]) * 0.5f;
        v[3] = t[0];
        v[4] = t[1];
        v[5] = t[2];
        v[6] = (t[0] + t[3]) * 0.5f;
        v[7] = (t[1] + t[4]) * 0.5f;
        v[8] = (t[2] + t[5]) * 0.5f;
        vert = _icosahedron_subdivide(v, level - 1, vert);

        v[0] = (t[0] + t[3]) * 0.5f;
        v[1] = (t[1] + t[4]) * 0.5f;
        v[2] = (t[2] + t[5]) * 0.5f;
        v[3] = t[3];
        v[4] = t[4];
        v[5] = t[5];
        v[6] = (t[3] + t[6]) * 0.5f;
        v[7] = (t[4] + t[7]) * 0.5f;
        v[8] = (t[5] + t[8]) * 0.5f;
        vert = _icosahedron_subdivide(v, level - 1, vert);

        v[0] = (t[6] + t[3]) * 0.5f;
        v[1] = (t[7] + t[4]) * 0.5f;
        v[2] = (t[8] + t[5]) * 0.5f;
        v[3] = t[6];
        v[4] = t[7];
        v[5] = t[8];
        v[6] = (t[6] + t[0]) * 0.5f;
        v[7] = (t[7] + t[1]) * 0.5f;
        v[8] = (t[8] + t[2]) * 0.5f;
        vert = _icosahedron_subdivide(v, level - 1, vert);

        v[0] = (t[0] + t[3]) * 0.5f;
        v[1] = (t[1] + t[4]) * 0.5f;
        v[2] = (t[2] + t[5]) * 0.5f;
        v[3] = (t[3] + t[6]) * 0.5f;
        v[4] = (t[4] + t[7]) * 0.5f;
        v[5] = (t[5] + t[8]) * 0.5f;
        v[6] = (t[0] + t[6]) * 0.5f;
        v[7] = (t[1] + t[7]) * 0.5f;
        v[8] = (t[2] + t[8]) * 0.5f;
        vert = _icosahedron_subdivide(v, level - 1, vert);
    }

    return vert;
}

void icosahedron_verts(unsigned subdivs, float *verts) {
    float *coords = verts;
    for (int i = 0; i < 20; ++i)
        coords = _icosahedron_subdivide(ICOSAHEDRON_VERTS + i * 9, subdivs, coords);
    int verts_count = icosahedron_verts_count(subdivs);
    for (int i = 0; i < verts_count; ++i) {
        float *v = verts + i * 3;
        float l = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        for (int j = 0; j < 3; ++j)
            v[j] /= l;
    }
}
