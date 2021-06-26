
void agent_see(global Agent *a, global Agent *b, constant SeeContext *c) {
    float4 a_p = float4_agent_position(a);
    float4 b_p = float4_agent_position(b);

    float4 d = float4_sub(b_p, a_p);
    float dl = float4_length(d);

    if (dl >= c->r) /* narrow narrow phase */
        return;

    /* forces are only calculated for neighbors in front of the boid */

    float dot = float4_dot(a->dir, d);
    if (dot > 0.0f) {

        /* alignment */

        a->alignment = float4_add(a->alignment, b->dir);

        /* separation */

        if (dl < c->separation_r) {
            a->separation = float4_sub(a->separation, float4_normalize_to(d, c->separation_r - dl));
            ++a->separation_count;
        }

        /* cohesion */

        a->cohesion = float4_add(a->cohesion, d);
        ++a->cohesion_count;
    }
}

void agent_act(global Agent *a, constant ActContext *c) {
    float4 p = float4_agent_position(a);
    float4 acc = float4_null();

    /* gravity */

    const float gravity_r = 400.0f;
    const float gravity_a = 0.00001f;

    float d = (float)sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    if (d > gravity_r)
        acc = float4_sub(acc, float4_mul_scalar(p, (d - gravity_r) * gravity_a / d));

    /* alignment */

    const float alignment_a = 0.02f;

    if (a->cohesion_count)
        acc = float4_add(acc, float4_mul_scalar(a->alignment, alignment_a / (float)a->cohesion_count));

    /* separation */

    const float separation_a = 0.02f;

    if (a->separation_count)
        acc = float4_add(acc, float4_mul_scalar(a->separation, separation_a / (float)a->separation_count));

    /* cohesion */

    const float cohesion_a = 0.001f;

    if (a->cohesion_count)
        acc = float4_add(acc, float4_mul_scalar(a->cohesion, cohesion_a / (float)a->cohesion_count));

    /* update */

    a->dir = float4_normalize(float4_add(a->dir, acc));
    float4_agent_position(a) = float4_add(p, float4_mul_scalar(a->dir, a->speed));

    a->separation = float4_null();
    a->alignment = float4_null();
    a->cohesion = float4_null();
    a->cohesion_count = 0;
    a->separation_count = 0;
}
