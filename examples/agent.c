
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

    // const float min_speed = 0.2f;
    // const float max_speed = 0.4f;

    a->dir = float4_normalize(float4_add(a->dir, acc));
    // float dir_acc = float4_dot(a->dir, acc) * 0.01f;
    // a->speed += dir_acc;
    // if (a->speed < min_speed)
    //     a->speed = min_speed;
    // else if (a->speed > max_speed)
    //     a->speed = max_speed;
    float4_agent_position(a) = float4_add(p, float4_mul_scalar(a->dir, a->speed));

    a->separation = float4_null();
    a->alignment = float4_null();
    a->cohesion = float4_null();
    a->cohesion_count = 0;
    a->separation_count = 0;
}

void sph_particle_density(global SphParticle *a, global SphParticle *b, constant SphContext *c) {
    float dx = b->p.x - a->p.x;
    float dy = b->p.y - a->p.y;
    float dz = b->p.z - a->p.z;
    float r2 = dx * dx + dy * dy + dz * dz;
    float z = c->h2 - r2;
    if (z > 0.0f) // (1.f - clamp(floor(r / h), 0.f, 1.f))
        a->density += c->poly6 * z * z * z;
}

void sph_particle_pressure(global SphParticle *a, constant SphContext *c) {
    /* Tait equation more suitable to liquids than state equation */
    a->pressure = c->K * ((float)pow(a->density / c->density, 7.0f) - 1.0f);

    /* state equation */
    // a->pressure = c->K * (a->density - c->density);
}

void sph_force_terms(global SphParticle *a, global SphParticle *b, constant SphContext *c) {
    float4 r;
    r.x = b->p.x - a->p.x;
    r.y = b->p.y - a->p.y;
    r.z = b->p.z - a->p.z;
    float r2 = r.x * r.x + r.y * r.y + r.z * r.z;

    float rl = (float)sqrt(r2);

    if (rl < c->h) {

        float spiky_gradient;
        if (rl < 0.00001f)
            spiky_gradient = c->spiky * c->h2;
        else
            spiky_gradient = c->spiky * (c->h - rl) * (c->h - rl);

        if (rl > 0.00001f)
            a->pressure_accum = float4_add(a->pressure_accum, float4_mul_scalar(r, spiky_gradient * (a->pressure + b->pressure) / (2.0f * b->density) / rl));

        a->viscosity_accum = float4_add(a->viscosity_accum, float4_mul_scalar(float4_sub(b->v, a->v), c->viscosity * (c->h - rl) / b->density));
    }
}

void sph_particle_leapfrog(global SphParticle *a, constant SphContext *c) {

    /* calculate internal forces */

    float4 f = float4_add(a->pressure_accum, float4_mul_scalar(a->viscosity_accum, c->dynamic_viscosity));

    /* acceleration */

    float4 acc = float4_div_scalar(f, a->density);
    acc.z -= 9.81f;

    /* leapfrog */

    a->vh = float4_add(a->vh, float4_mul_scalar(acc, c->dt));

    a->v = float4_add(a->vh, float4_mul_scalar(acc, c->dt * 0.5f));
    a->p = float4_add(a->p, float4_mul_scalar(a->vh, c->dt));

    const float damp = -0.5f;

    if (a->p.x < c->min.x) {
        a->p.x = c->min.x;
        a->v.x = damp * a->v.x;
        a->vh.x = damp * a->vh.x;
    }

    if (a->p.y < c->min.y) {
        a->p.y = c->min.y;
        a->v.y = damp * a->v.y;
        a->vh.y = damp * a->vh.y;
    }

    if (a->p.z < c->min.z) {
        a->p.z = c->min.z;
        a->v.z = damp * a->v.z;
        a->vh.z = damp * a->vh.z;
    }

    if (a->p.x > c->max.x) {
        a->p.x = c->max.x;
        a->v.x = damp * a->v.x;
        a->vh.x = damp * a->vh.x;
    }

    if (a->p.y > c->max.y) {
        a->p.y = c->max.y;
        a->v.y = damp * a->v.y;
        a->vh.y = damp * a->vh.y;
    }

    if (a->p.z > c->max.z) {
        a->p.z = c->max.z;
        a->v.z = damp * a->v.z;
        a->vh.z = damp * a->vh.z;
    }

    sph_particle_reset(a);
}

void sph_particle_reset(global SphParticle *a) {
    a->density = 0.0;
    a->pressure_accum.x = 0.0f;
    a->pressure_accum.y = 0.0f;
    a->pressure_accum.z = 0.0f;
    a->viscosity_accum.x = 0.0f;
    a->viscosity_accum.y = 0.0f;
    a->viscosity_accum.z = 0.0f;
}

/*
** Particle-in-cell flocking
*/

void pic_reset_node(global PicBoidNode *n, global void *c) {
    n->p_sum = float4_null();
    n->dir_sum = float4_null();
    n->density = 0.0f;
    n->count = 0.0f;
}

void pic_transfer_boid_to_node(global PicBoid *a, global PicBoidNode *n, constant PicFlockingContext *c) {
    float4 d = float4_sub(a->p, n->p);
    float dl = float4_length(d);

    if (dl >= c->radius || dl < 0.00001f)
        return;

    float w = 1.0f - dl / c->radius;

    tay_atomic_add_float(&n->p_sum.x, d.x);
    tay_atomic_add_float(&n->p_sum.y, d.y);
    tay_atomic_add_float(&n->p_sum.z, d.z);

    tay_atomic_add_float(&n->dir_sum.x, a->dir.x);
    tay_atomic_add_float(&n->dir_sum.y, a->dir.y);
    tay_atomic_add_float(&n->dir_sum.z, a->dir.z);
    // float4 dir_w = float4_mul_scalar(a->dir, w);
    // tay_atomic_add_float(&n->dir_sum.x, dir_w.x);
    // tay_atomic_add_float(&n->dir_sum.y, dir_w.y);
    // tay_atomic_add_float(&n->dir_sum.z, dir_w.z);

    tay_atomic_add_float(&n->count, 1.0f);
}

void pic_transfer_node_to_boids(global PicBoid *a, global PicBoidNode *n, constant PicFlockingContext *c) {
    float4 d = float4_sub(a->p, n->p);
    float dl = float4_length(d);

    if (dl >= c->radius || dl < 0.00001f)
        return;

    float w = 1.0f - dl / c->radius;

    a->dir_sum = float4_add(a->dir_sum, float4_mul_scalar(n->dir_sum, w));

    float separation_r = c->radius * 0.5f;

    float4 n_avg_p = float4_add(float4_div_scalar(n->p_sum, n->count), n->p);
    float4 dp = float4_sub(a->p, n_avg_p);
    float dpl = float4_length(dp);

    if (dpl < separation_r && dpl > 0.00001f)
        a->sep_f = float4_add(a->sep_f, float4_div_scalar(dp, dpl));

    a->coh_p = float4_add(a->coh_p, dp);
    ++a->coh_count;
}

void pic_boid_action(global PicBoid *a, constant PicFlockingContext *c) {
    const float speed = 1.0f;

    const float alignment = 0.02f;

    float dir_sum_length = float4_length(a->dir_sum);
    if (dir_sum_length > 0.00001f)
        a->dir = float4_add(a->dir, float4_mul_scalar(a->dir_sum, alignment / dir_sum_length));

    const float separation = 0.01f;

    float sep_f_l = float4_length(a->sep_f);
    if (sep_f_l > 0.00001f)
        a->dir = float4_add(a->dir, float4_mul_scalar(a->sep_f, separation / sep_f_l));

    const float cohesion = -0.01f;

    float coh_p_l = float4_length(a->coh_p);
    if (coh_p_l > 0.00001f)
        a->dir = float4_add(a->dir, float4_mul_scalar(a->coh_p, cohesion / coh_p_l));

    a->dir = float4_normalize(a->dir);

    a->p = float4_add(a->p, float4_mul_scalar(a->dir, speed));
    a->dir_sum = float4_null();
    a->sep_f = float4_null();
    a->coh_p = float4_null();
    a->coh_count = 0;

    if (a->p.x < c->min.x) {
        a->p.x = c->min.x;
        a->dir.x = -a->dir.x;
    }
    if (a->p.y < c->min.y) {
        a->p.y = c->min.y;
        a->dir.y = -a->dir.y;
    }
    if (a->p.z < c->min.z) {
        a->p.z = c->min.z;
        a->dir.z = -a->dir.z;
    }

    if (a->p.x > c->max.x) {
        a->p.x = c->max.x;
        a->dir.x = -a->dir.x;
    }
    if (a->p.y > c->max.y) {
        a->p.y = c->max.y;
        a->dir.y = -a->dir.y;
    }
    if (a->p.z > c->max.z) {
        a->p.z = c->max.z;
        a->dir.z = -a->dir.z;
    }
}
