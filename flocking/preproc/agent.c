void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *c) {
    float3 a_p = float3_agent_position(a);
    float3 b_p = float3_agent_position(b);

    float3 d = float3_sub(b_p, a_p);
    float dl = float3_length(d);

    if (dl >= c->r) /* narrow narrow phase */
        return;

    /* forces are only calculated for neighbors in front of the boid */

    float dot = float3_dot(a->dir, d);
    if (dot > 0.0f) {

        /* alignment */

        a->alignment = float3_add(a->alignment, b->dir);

        /* separation */

        if (dl < c->separation_r) {
            float3 s = float3_sub(d, float3_mul_scalar(a->dir, -dot));
            s = float3_normalize_to(s, c->separation_r - float3_length(s));
            a->separation = float3_sub(a->separation, s);
        }

        /* cohesion */

        a->cohesion = float3_add(a->cohesion, d);
        ++a->cohesion_count;
    }
}

void agent_act(__GLOBAL__ Agent *a, __GLOBAL__ ActContext *c) {
    float3 p = float3_agent_position(a);
    float3 acc = float3_null();

    /* gravity */

    const float gravity_r = 200.0f;
    const float gravity_a = 0.00001f;

    float d = (float)sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    if (d > gravity_r)
        acc = float3_sub(acc, float3_mul_scalar(p, (d - gravity_r) * gravity_a / d));

    /* alignment */

    const float alignment_a = 0.01f;

    acc = float3_add(acc, float3_normalize_to(a->alignment, alignment_a));

    /* separation */

    const float separation_a = 0.03f;

    acc = float3_add(acc, float3_normalize_to(a->separation, separation_a));

    /* cohesion */

    const float cohesion_a = 0.04f;

    acc = float3_add(acc, float3_normalize_to(a->cohesion, cohesion_a));

    /* update */

    a->dir = float3_normalize(float3_add(a->dir, acc));
    float3_agent_position(a) = float3_add(p, float3_mul_scalar(a->dir, a->speed));

    a->separation = float3_null();
    a->alignment = float3_null();
    a->cohesion = float3_null();
    a->cohesion_count = 0;
}
