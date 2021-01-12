void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *c) {
    float3 a_p = float3_agent_position(a);
    float3 b_p = float3_agent_position(b);

    float dx = b_p.x - a_p.x;
    float dy = b_p.y - a_p.y;
    float dz = b_p.z - a_p.z;
    float d_sq = dx * dx + dy * dy + dz * dz;

    if (d_sq >= c->r_sq) /* narrow narrow phase */
        return;

    float d = (float)sqrt(d_sq);

    float f = 0.0f;
    if (d < c->r1)
        f = c->repulsion;
    else if (d < c->r2)
        f = c->repulsion + (c->attraction - c->repulsion) * (d - c->r1) / (c->r2 - c->r1);
    else
        f = c->attraction + c->attraction * (1.0f - (d - c->r2) / (c->r - c->r2));

    a->f.x += f * dx / d;
    a->f.y += f * dy / d;
    a->f.z += f * dz / d;

    ++a->seen;
}

void agent_act(__GLOBAL__ Agent *a, __GLOBAL__ ActContext *c) {
    float3 p = float3_agent_position(a);

    /* calculate force */

    const float gravity = 0.000001f;

    float3 f;
    if (a->seen) {
        f = float3_mul_scalar(float3_div_scalar(a->f, (float)a->seen), 0.2f);
        f.x -= p.x * gravity;
        f.y -= p.y * gravity;
        f.z -= p.z * gravity;
    }
    else {
        f.x = -p.x * gravity;
        f.y = -p.y * gravity;
        f.z = -p.z * gravity;
    }

    /* velocity from force */

    const float min_v = 0.2f;
    const float max_v = 0.3f;

    a->v = float3_add(a->v, f);
    float v = float3_length(a->v);

    if (v < 0.000001f) {
        a->v.x = min_v;
        a->v.y = 0.0f;
        a->v.z = 0.0f;
    }
    else if (v < min_v) {
        a->v.x *= min_v / v;
        a->v.y *= min_v / v;
        a->v.z *= min_v / v;
    }
    else if (v > max_v) {
        a->v.x *= max_v / v;
        a->v.y *= max_v / v;
        a->v.z *= max_v / v;
    }

    float3_agent_position(a) = float3_add(p, a->v);

    a->f = float3_null();
    a->seen = 0;
}
