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
            a->separation = float3_sub(a->separation, float3_normalize_to(d, c->separation_r - dl));
            ++a->separation_count;
        }

        /* cohesion */

        a->cohesion = float3_add(a->cohesion, d);
        ++a->cohesion_count;
    }
}

void agent_obstacle_see(__GLOBAL__ Agent *a, __GLOBAL__ Obstacle *b, __GLOBAL__ SeeContext *c) {
    float3 a_p = float3_agent_position(a);
    float3 b_min = float3_agent_min(b);
    float3 b_max = float3_agent_max(b);

    float3 b_p;
    b_p.x = (b_min.x + b_max.x) * 0.5f;
    b_p.y = (b_min.y + b_max.y) * 0.5f;
    b_p.z = (b_min.z + b_max.z) * 0.5f;

    float3 d = float3_sub(a_p, b_p);
    float dl = float3_length(d) - b->radius;

    if (dl < c->avoidance_r) {
        float f = c->avoidance_c * (c->avoidance_r - dl);
        a->avoidance = float3_add(a->avoidance, float3_normalize_to(d, f));
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

    const float alignment_a = 0.02f;

    if (a->cohesion_count)
        acc = float3_add(acc, float3_mul_scalar(a->alignment, alignment_a / (float)a->cohesion_count));

    /* separation */

    const float separation_a = 0.02f;

    if (a->separation_count)
        acc = float3_add(acc, float3_mul_scalar(a->separation, separation_a / (float)a->separation_count));

    /* cohesion */

    const float cohesion_a = 0.001f;

    if (a->cohesion_count)
        acc = float3_add(acc, float3_mul_scalar(a->cohesion, cohesion_a / (float)a->cohesion_count));

    /* obstacle avoidance */

    acc = float3_add(acc, a->avoidance);

    /* update */

    // const float min_speed = 0.2f;
    // const float max_speed = 0.4f;

    a->dir = float3_normalize(float3_add(a->dir, acc));
    // float dir_acc = float3_dot(a->dir, acc) * 0.01f;
    // a->speed += dir_acc;
    // if (a->speed < min_speed)
    //     a->speed = min_speed;
    // else if (a->speed > max_speed)
    //     a->speed = max_speed;
    float3_agent_position(a) = float3_add(p, float3_mul_scalar(a->dir, a->speed));

    a->separation = float3_null();
    a->alignment = float3_null();
    a->cohesion = float3_null();
    a->avoidance = float3_null();
    a->cohesion_count = 0;
    a->separation_count = 0;
}
