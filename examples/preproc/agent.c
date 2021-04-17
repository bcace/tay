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

void sph_particle_density(__GLOBAL__ SphParticle *a, __GLOBAL__ SphParticle *b, __GLOBAL__ SphContext *c) {
    float dx = b->p.x - a->p.x;
    float dy = b->p.y - a->p.y;
    float dz = b->p.z - a->p.z;
    float r2 = dx * dx + dy * dy + dz * dz;
    float z = c->h2 - r2;
    if (z > 0.0f) // (1.f - clamp(floor(r / h), 0.f, 1.f))
        a->density += c->poly6 * z * z * z;
}

void sph_particle_acceleration(__GLOBAL__ SphParticle *a, __GLOBAL__ SphParticle *b, __GLOBAL__ SphContext *c) {
    float3 r = float3_sub(b->p.xyz, a->p.xyz);
    float rl = float3_length(r);

    if (rl < c->h) {

        if (a != b) {

            float3 spiky_gradient;
            if (rl < 0.00001f) { // TODO: is this widening OK?
                spiky_gradient.x = c->spiky * c->h2;
                spiky_gradient.y = c->spiky * c->h2;
                spiky_gradient.z = c->spiky * c->h2;
            }
            else
                spiky_gradient = float3_mul_scalar(r, c->spiky * (c->h - rl) * (c->h - rl) / rl);

            a->pressure_accum = float3_add(
                                    a->pressure_accum,
                                    float3_mul(
                                        float3_add(
                                            float3_div_scalar(a->pressure, (a->density * a->density)),
                                            float3_div_scalar(b->pressure, (b->density * b->density))
                                        ),
                                        spiky_gradient
                                    )
                                );

            float viscosity_laplacian = c->viscosity * (c->h - rl);

            a->viscosity_accum = float3_add(a->viscosity_accum, float3_mul_scalar(float3_sub(b->v, a->v), viscosity_laplacian / b->density));
        }

        float3 poly6_gradient = float3_mul_scalar(r, c->poly6_gradient * (c->h2 - rl * rl) * (c->h2 - rl * rl) / b->density);

        a->normal = float3_add(a->normal, poly6_gradient);

        a->color_field_laplacian += c->poly6_laplacian * (c->h2 - rl * rl) * (3.0f * c->h2 - 7.0f * rl * rl) / b->density;
    }
}

void sph_particle_leapfrog(__GLOBAL__ SphParticle *a, __GLOBAL__ SphContext *c) {

    /* calculate internal forces */

    float3 f = float3_add(float3_mul_scalar(a->pressure_accum, -a->density), float3_mul_scalar(a->viscosity_accum, c->dynamic_viscosity));

    float nl = float3_length(a->normal);
    if (nl > c->surface_tension_threshold)
        f = float3_add(f, float3_mul_scalar(a->normal, -c->surface_tension * a->color_field_laplacian / nl));

    /* acceleration */

    float3 acc = float3_div_scalar(f, a->density);
    acc.z -= 9.81f;

    /* leapfrog */

    a->vh = float3_add(a->vh, float3_mul_scalar(acc, c->dt));
    a->v = float3_add(a->vh, float3_mul_scalar(acc, c->dt * 0.5f));
    a->p.xyz = float3_add(a->p.xyz, float3_mul_scalar(a->vh, c->dt));

    const float damp = 0.75f;

    if (a->p.x < c->min.x) {
        a->p.x = c->min.x;
        a->v.x = -a->v.x;
        a->vh.x = -a->vh.x;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.y < c->min.y) {
        a->p.y = c->min.y;
        a->v.y = -a->v.y;
        a->vh.y = -a->vh.y;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.z < c->min.z) {
        a->p.z = c->min.z;
        a->v.z = -a->v.z;
        a->vh.z = -a->vh.z;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.x > c->max.x) {
        a->p.x = c->max.x;
        a->v.x = -a->v.x;
        a->vh.x = -a->vh.x;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.y > c->max.y) {
        a->p.y = c->max.y;
        a->v.y = -a->v.y;
        a->vh.y = -a->vh.y;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    if (a->p.z > c->max.z) {
        a->p.z = c->max.z;
        a->v.z = -a->v.z;
        a->vh.z = -a->vh.z;
        // a->v = float3_mul_scalar(a->v, damp);
        // a->vh = float3_mul_scalar(a->vh, damp);
    }

    sph_particle_reset(a);
}

void sph_particle_reset(__GLOBAL__ SphParticle *a) {
    a->density = 0.0;
    a->color_field_laplacian = 0.0f;
    a->pressure_accum.x = 0.0f;
    a->pressure_accum.y = 0.0f;
    a->pressure_accum.z = 0.0f;
    a->viscosity_accum.x = 0.0f;
    a->viscosity_accum.y = 0.0f;
    a->viscosity_accum.z = 0.0f;
    a->normal.x = 0.0f;
    a->normal.y = 0.0f;
    a->normal.z = 0.0f;
}
