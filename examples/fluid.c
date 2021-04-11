#include "main.h"
#include "tay.h"
#include "agent.h"
#include "shaders.h"
#include "graphics.h"
#include <math.h>
#include <stdlib.h>


static TayGroup *particles_group;
static ParticleSeeContext particle_see_context;
static ParticleActContext particle_act_context;

static int particles_count = 10000;

static float icosahedron[] = {
    0.5257f, 0.0f, 0.8506f, 0.0f, 0.8506f, 0.5257f, -0.5257f, 0.0f, 0.8506f,
    0.0f, 0.8506f, 0.5257f, -0.8506f, 0.5257f, 0.0f, -0.5257f, 0.0f, 0.8506f,
    0.0f, 0.8506f, 0.5257f, 0.0f, 0.8506f, -0.5257f, -0.8506f, 0.5257f, 0.0f,
    0.8506f, 0.5257f, 0.0f, 0.0f, 0.8506f, -0.5257f, 0.0f, 0.8506f, 0.5257f,
    0.5257f, 0.0f, 0.8506f, 0.8506f, 0.5257f, 0.0f, 0.0f, 0.8506f, 0.5257f,
    0.5257f, 0.0f, 0.8506f, 0.8506f, -0.5257f, 0.0f, 0.8506f, 0.5257f, 0.0f,
    0.8506f, -0.5257f, 0.0f, 0.5257f, 0.0f, -0.8506f, 0.8506f, 0.5257f, 0.0f,
    0.8506f, 0.5257f, 0.0f, 0.5257f, 0.0f, -0.8506f, 0.0f, 0.8506f, -0.5257f,
    0.5257f, 0.0f, -0.8506f, -0.5257f, 0.0f, -0.8506f, 0.0f, 0.8506f, -0.5257f,
    0.5257f, 0.0f, -0.8506f, 0.0f, -0.8506f, -0.5257f, -0.5257f, 0.0f, -0.8506f,
    0.5257f, 0.0f, -0.8506f, 0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, -0.5257f,
    0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, 0.5257f, 0.0f, -0.8506f, -0.5257f,
    0.0f, -0.8506f, 0.5257f, -0.8506f, -0.5257f, 0.0f, 0.0f, -0.8506f, -0.5257f,
    0.0f, -0.8506f, 0.5257f, -0.5257f, 0.0f, 0.8506f, -0.8506f, -0.5257f, 0.0f,
    0.0f, -0.8506f, 0.5257f, 0.5257f, 0.0f, 0.8506f, -0.5257f, 0.0f, 0.8506f,
    0.8506f, -0.5257f, 0.0f, 0.5257f, 0.0f, 0.8506f, 0.0f, -0.8506f, 0.5257f,
    -0.8506f, -0.5257f, 0.0f, -0.5257f, 0.0f, 0.8506f, -0.8506f, 0.5257f, 0.0f,
    -0.5257f, 0.0f, -0.8506f, -0.8506f, -0.5257f, 0.0f, -0.8506f, 0.5257f, 0.0f,
    0.0f, 0.8506f, -0.5257f, -0.5257f, 0.0f, -0.8506f, -0.8506f, 0.5257f, 0.0f,
    -0.8506f, -0.5257f, 0.0f, -0.5257f, 0.0f, -0.8506f, 0.0f, -0.8506f, -0.5257f,
};

static const int icosahedron_verts_count = 180;

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

void fluid_init() {

    const float r = 10.0f;

    particles_group = tay_add_group(global.tay, sizeof(Particle), particles_count, TAY_TRUE);
    tay_configure_space(global.tay, particles_group, TAY_CPU_GRID, 3, (float4){r, r, r, r}, 250);

    particle_see_context.r = r;

    particle_act_context.min.x = -100.0f;
    particle_act_context.min.y = -100.0f;
    particle_act_context.min.z = -100.0f;
    particle_act_context.max.x = 100.0f;
    particle_act_context.max.y = 100.0f;
    particle_act_context.max.z = 100.0f;

    tay_add_see(global.tay, particles_group, particles_group, particle_see, (float4){r, r, r, r}, &particle_see_context);
    tay_add_act(global.tay, particles_group, particle_act, &particle_act_context);

    for (int i = 0; i < particles_count; ++i) {
        Particle *p = tay_get_available_agent(global.tay, particles_group);
        p->p.x = _rand(particle_act_context.min.x, particle_act_context.max.x);
        p->p.y = _rand(particle_act_context.min.y, particle_act_context.max.y);
        p->p.z = _rand(particle_act_context.min.z, particle_act_context.max.z);
        p->v = (float3){0.0f, 0.0f, 0.0f};
        p->f = (float3){0.0f, 0.0f, 0.0f};
        tay_commit_available_agent(global.tay, particles_group);
    }

}

void fluid_draw() {
}
