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
}

void agent_act(__GLOBAL__ Agent *a, __GLOBAL__ ActContext *c) {
    a->v = float3_add(a->v, float3_mul_scalar(float3_normalize(a->f), 0.001f));
    float v = float3_length(a->v);

    const float min_v = 0.1f;
    const float max_v = 0.2f;

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
    float3_agent_position(a) = float3_add(float3_agent_position(a), a->v);
    a->f = float3_null();
}
