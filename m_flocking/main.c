#include "entorama.h"
#include "tay.h"
#include "agent_host.h"
#include <math.h>
#include <stdlib.h>


static TayGroup *boids_group;

static ActContext act_context;
static SeeContext see_context;

static int boids_count = 30000;

static float _rand(float min, float max) {
    return min + rand() * (max - min) / (float)RAND_MAX;
}

int entorama_init(EntoramaSimulationInfo *info, TayState *tay) {
    const float radius = 20.0f;
    const float avoidance_r = 100.0f;
    const float4 see_radii = { radius, radius, radius, radius };
    const float part_size = radius * 1.0f;
    const float4 part_sizes = { part_size, part_size, part_size, part_size };
    const float velocity = 1.0f;
    const float4 min = { -300.0f, -300.0f, -300.0f, -300.0f };
    const float4 max = { 300.0f, 300.0f, 300.0f, 300.0f };

    see_context.r = radius;
    see_context.separation_r = radius * 0.5f;

    boids_group = tay_add_group(tay, sizeof(Agent), boids_count, TAY_TRUE);
    EntoramaGroupInfo *group_info = info->groups + info->groups_count++;
    group_info->group = boids_group;
    group_info->max_agents = boids_count;

    tay_configure_space(tay, boids_group, TAY_CPU_GRID, 3, part_sizes, 250);

    tay_add_see(tay, boids_group, boids_group, agent_see, "agent_see", see_radii, TAY_FALSE, &see_context, sizeof(see_context));
    tay_add_act(tay, boids_group, agent_act, "agent_act", &act_context, sizeof(act_context));

    for (int i = 0; i < boids_count; ++i) {
        Agent *boid = tay_get_available_agent(tay, boids_group);
        boid->p.x = _rand(min.x, max.x);
        boid->p.y = _rand(min.y, max.y);
        boid->p.z = _rand(min.z, max.z);
        boid->dir.x = _rand(-1.0f, 1.0f);
        boid->dir.y = _rand(-1.0f, 1.0f);
        boid->dir.z = _rand(-1.0f, 1.0f);
        float l = velocity / sqrtf(boid->dir.x * boid->dir.x + boid->dir.y * boid->dir.y + boid->dir.z * boid->dir.z);
        boid->dir.x *= l;
        boid->dir.y *= l;
        boid->dir.z *= l;
        boid->speed = 1.0f;
        boid->separation = (float4){0.0f, 0.0f, 0.0f, 0.0f};
        boid->alignment = (float4){0.0f, 0.0f, 0.0f, 0.0f};
        boid->cohesion = (float4){0.0f, 0.0f, 0.0f, 0.0f};
        boid->cohesion_count = 0;
        boid->separation_count = 0;
        tay_commit_available_agent(tay, boids_group);
    }

    // ocl_add_source(tay, "agent.h");
    // ocl_add_source(tay, "taystd.h");
    // ocl_add_source(tay, "agent.c");
    // ocl_add_source(tay, "taystd.c");

    return 0;
}

__declspec(dllexport) int entorama_main(EntoramaModelInfo *info) {
    info->init = entorama_init;
    info->origin_x = 0.0f;
    info->origin_y = 0.0f;
    info->origin_z = 0.0f;
    info->radius = 400.0f;
    return 0;
}
