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
    a->cohesion_count = 0;
    a->separation_count = 0;
}

void particle_see(__GLOBAL__ Particle *a, __GLOBAL__ Particle *b, __GLOBAL__ ParticleSeeContext *c) {
    float3 d = float3_sub(b->p.xyz, a->p.xyz);
    float l = float3_length(d);
    if (l > 0.0f && l < c->r)
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - l) / l));
}

void particle_act(__GLOBAL__ Particle *a, __GLOBAL__ ParticleActContext *c) {

    const float m = 0.01f;
    const float t = 0.11f;

    float3 acc = float3_mul_scalar(a->f, 0.8f / m);                 // acceleration
    acc.z -= 30.0f;                                                 // gravity
    a->v = float3_add(a->v, float3_mul_scalar(acc, 0.5f * t));      // velocity increment
    a->p.xyz = float3_add(a->p.xyz, float3_mul_scalar(a->v, t));    // move
    a->v = float3_mul_scalar(a->v, 0.96f);                          // dissipate
    a->f = float3_null();                                           // reset force

    if (a->p.x < c->min.x) {
        a->p.x = c->min.x;
        a->v.x = -a->v.x;
    }

    if (a->p.y < c->min.y) {
        a->p.y = c->min.y;
        a->v.y = -a->v.y;
    }

    if (a->p.z < c->min.z) {
        a->p.z = c->min.z;
        a->v.z = -a->v.z;
    }

    if (a->p.x > c->max.x) {
        a->p.x = c->max.x;
        a->v.x = -a->v.x;
    }

    if (a->p.y > c->max.y) {
        a->p.y = c->max.y;
        a->v.y = -a->v.y;
    }

    if (a->p.z > c->max.z) {
        a->p.z = c->max.z;
        a->v.z = -a->v.z;
    }
}

void ball_particle_see(__GLOBAL__ Ball *a, __GLOBAL__ Particle *b, __GLOBAL__ BallParticleSeeContext *c) {
    float3 a_p = float3_mul_scalar(float3_add(a->min.xyz, a->max.xyz), 0.5f);
    float3 d = float3_sub(b->p.xyz, a_p);
    float l = float3_length(d);
    float dl = l - c->ball_r;
    if (dl < c->r)
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - dl) / l));
}

void particle_ball_see(__GLOBAL__ Particle *a, __GLOBAL__ Ball *b, __GLOBAL__ BallParticleSeeContext *c) {
    float3 b_p = float3_mul_scalar(float3_add(b->min.xyz, b->max.xyz), 0.5f);
    float3 d = float3_sub(b_p, a->p.xyz);
    float l = float3_length(d);
    float dl = l - c->ball_r;
    if (dl < c->r)
        a->f = float3_sub(a->f, float3_mul_scalar(d, (c->r - dl) / l));
}

void ball_act(__GLOBAL__ Ball *a, __GLOBAL__ void *c) {

    const float m = 40.1f;
    const float t = 0.11f; // TODO: move to context

    float3 acc = float3_mul_scalar(a->f, 0.8f / m);                 // acceleration
    acc.z -= 30.0f;                                                 // gravity
    a->v = float3_add(a->v, float3_mul_scalar(acc, 0.5f * t));      // velocity increment
    float3 d = float3_mul_scalar(a->v, t);
    a->min.xyz = float3_add(a->min.xyz, d);
    a->max.xyz = float3_add(a->max.xyz, d);
    a->v = float3_mul_scalar(a->v, 0.96f);                          // dissipate
    a->f = float3_null();                                           // reset force

    if (a->max.z < -100.0f) {
        a->min.z += 2000.0f;
        a->max.z += 2000.0f;
    }
}
