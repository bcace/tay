void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *c) {
    float3 a_p = float3_get_agent_position(a);
    float3 b_p = float3_get_agent_position(b);
    for (int i = 0; i < 1; ++i)
        a->b_buffer = float3_add(a->b_buffer, float3_sub(b_p, a_p));
    a->b_buffer_count++;
}

void agent_act(__GLOBAL__ Agent *agent, __GLOBAL__ ActContext *c) {

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        float3 n = float3_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->f_buffer = float3_add(agent->f_buffer, n);
        agent->b_buffer = float3_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    float3 p = float3_get_agent_position(agent);
    float3_set_agent_position(agent, float3_add(p, agent->v));

    if (agent->p.x < c->min.x) {
        agent->p.x = c->min.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->p.y < c->min.y) {
        agent->p.y = c->min.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->p.z < c->min.z) {
        agent->p.z = c->min.z;
        agent->v.z = -agent->v.z;
    }

    if (agent->p.x > c->max.x) {
        agent->p.x = c->max.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->p.y > c->max.y) {
        agent->p.y = c->max.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->p.z > c->max.z) {
        agent->p.z = c->max.z;
        agent->v.z = -agent->v.z;
    }
}
