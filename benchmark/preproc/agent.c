void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *c) {
    float4 a_p = float4_agent_position(a);
    float4 b_p = float4_agent_position(b);
    for (int i = 0; i < 1; ++i)
        a->b_buffer = float4_add(a->b_buffer, float4_sub(b_p, a_p));
    a->b_buffer_count++;
}

void agent_act(__GLOBAL__ Agent *agent, __GLOBAL__ ActContext *c) {

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        float4 n = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->f_buffer = float4_add(agent->f_buffer, n);
        agent->b_buffer = float4_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    float4 p = float4_agent_position(agent);
    float4_agent_position(agent) = float4_add(p, agent->v);

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

void box_agent_see(__GLOBAL__ BoxAgent *a, __GLOBAL__ BoxAgent *b, __GLOBAL__ SeeContext *c) {
    float4 a_min = float4_agent_min(a);
    float4 b_min = float4_agent_min(b);
    for (int i = 0; i < 1; ++i)
        a->b_buffer = float4_add(a->b_buffer, float4_sub(b_min, a_min));
    a->b_buffer_count++;
}

void box_agent_act(__GLOBAL__ BoxAgent *agent, __GLOBAL__ ActContext *c) {

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        float4 n = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->f_buffer = float4_add(agent->f_buffer, n);
        agent->b_buffer = float4_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    float4 min = float4_agent_min(agent);
    float4 max = float4_agent_max(agent);
    float4 size = float4_sub(max, min);
    float4_agent_min(agent) = float4_add(min, agent->v);
    float4_agent_max(agent) = float4_add(max, agent->v);

    if (agent->min.x < c->min.x) {
        agent->min.x = c->min.x;
        agent->max.x = c->min.x + size.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->min.y < c->min.y) {
        agent->min.y = c->min.y;
        agent->max.y = c->min.y + size.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->min.z < c->min.z) {
        agent->min.z = c->min.z;
        agent->max.z = c->min.z + size.z;
        agent->v.z = -agent->v.z;
    }

    if (agent->max.x > c->max.x) {
        agent->max.x = c->max.x;
        agent->min.x = c->max.x - size.x;
        agent->v.x = -agent->v.x;
    }

    if (agent->max.y > c->max.y) {
        agent->max.y = c->max.y;
        agent->min.y = c->max.y - size.y;
        agent->v.y = -agent->v.y;
    }

    if (agent->max.z > c->max.z) {
        agent->max.z = c->max.z;
        agent->min.z = c->max.z - size.z;
        agent->v.z = -agent->v.z;
    }
}
