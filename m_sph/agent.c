
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
