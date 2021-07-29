#include "entorama.h"
#include "tay.h"
#include "agent_host.h"
#include "agent_ocl.h"
#include <math.h>
#include <stdlib.h>

#define F_PI 3.14159265358979323846f


static TayGroup *particles_group;

static SphContext sph_context;

static int particles_count = 20000;

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

static void _update_sph_context(SphContext *c, float m) {
    c->m = m;

    float h2 = c->h * c->h;
    float h6 = h2 * h2 * h2;
    float h9 = h6 * h2 * c->h;

    c->h2 = h2;

    /* Matthias Müller, David Charypar and Markus Gross (2003) */
    c->poly6 = m * 315.0f / (64.0f * F_PI * h9);
    c->poly6_gradient = m * -945.0f / (32.0f * F_PI * h9);
    c->poly6_laplacian = c->poly6_gradient;
    c->spiky = m * -45.0f / (F_PI * h6);
    c->viscosity = -c->spiky;
}

static int _init(EntoramaModel *model, TayState *tay) {
    float particle_m = 0.05f;
    float fluid_density = 998.29f;
    float atmospheric_pressure = 101325.0f;
    float total_m = particles_count * particle_m;
    float initial_volume = total_m / fluid_density;

    int particles_inside_influence_radius = 20;

    sph_context.K = atmospheric_pressure / fluid_density; // from pV=mRT
    sph_context.density = fluid_density;
    sph_context.h = cbrtf(3.0f * (particles_inside_influence_radius * (initial_volume / particles_count)) / (4.0f * F_PI));
    sph_context.dt = 0.004f;
    sph_context.dynamic_viscosity = 3.5f;
    sph_context.surface_tension = 0.0728f;
    sph_context.surface_tension_threshold = 7.065f;
    sph_context.min = (float4){-3.0f, -2.0f, -1.0f, 0.0f};
    sph_context.max = (float4){3.0f, 2.0f, 1.0f, 0.0f};

    model->set_world_box(model, sph_context.min.x, sph_context.min.y, sph_context.min.z, sph_context.max.x, sph_context.max.y, sph_context.max.z);

    _update_sph_context(&sph_context, particle_m);

    float h = sph_context.h;
    float part_size = h * 1.0f;

    particles_group = tay_add_group(tay, sizeof(SphParticle), particles_count, TAY_TRUE);
    EntoramaGroup *e_particles_group = model->add_group(model, "Particles", particles_group, particles_count, 1);
    e_particles_group->group = particles_group;
    e_particles_group->max_agents = particles_count;
    e_particles_group->size_source = ENTORAMA_SIZE_UNIFORM_RADIUS;
    e_particles_group->size_radius = 0.02f;
    e_particles_group->shape = ENTORAMA_SPHERE;
    e_particles_group->configure_space(e_particles_group, TAY_CPU_GRID, part_size, part_size, part_size, part_size);

    // tay_group_enable_ocl(tay, particles_group);

    tay_add_see(tay, particles_group, particles_group, sph_particle_density, "sph_particle_density", (float4){h, h, h, h}, TAY_TRUE, &sph_context, sizeof(sph_context));
    tay_add_act(tay, particles_group, sph_particle_pressure, "sph_particle_pressure", &sph_context, sizeof(sph_context));
    tay_add_see(tay, particles_group, particles_group, sph_force_terms, "sph_force_terms", (float4){h, h, h, h}, TAY_FALSE, &sph_context, sizeof(sph_context));
    tay_add_act(tay, particles_group, sph_particle_leapfrog, "sph_particle_leapfrog", &sph_context, sizeof(sph_context));

    model->add_see(model, "Density", e_particles_group, e_particles_group);
    model->add_act(model, "Pressure", e_particles_group);
    model->add_see(model, "Force terms", e_particles_group, e_particles_group);
    model->add_act(model, "Leapfrog", e_particles_group);

    ocl_add_source(tay, agent_ocl_h);
    ocl_add_source(tay, taystd_ocl_h);
    ocl_add_source(tay, agent_ocl_c);
    ocl_add_source(tay, taystd_ocl_c);

    return 0;
}

static int _reset(EntoramaModel *model, TayState *tay) {
    tay_clear_all_agents(tay, particles_group);

    for (int i = 0; i < particles_count; ++i) {
        SphParticle *p = tay_get_available_agent(tay, particles_group);
        p->p.x = _rand(sph_context.min.x, sph_context.min.x + (sph_context.max.x - sph_context.min.x) * 0.5f);
        p->p.y = _rand(sph_context.min.y, sph_context.max.y);
        p->p.z = _rand(sph_context.min.z, sph_context.min.z + (sph_context.max.z - sph_context.min.z) * 0.5f);
        p->vh = (float4){0.0f, 0.0f, 0.0f, 0.0f};
        p->v = (float4){0.0f, 0.0f, 0.0f, 0.0f};
        sph_particle_reset(p);
        tay_commit_available_agent(tay, particles_group);
    }

    return 0;
}

__declspec(dllexport) int entorama_main(EntoramaModel *model) {
    model->init = _init;
    model->reset = _reset;
    return 0;
}
