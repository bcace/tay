void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *c) {
    a->b_buffer = float4_add(a->b_buffer, float4_sub(b->p, a->p));
    a->b_buffer_count++;
}

void agent_act(__GLOBAL__ Agent *agent, __GLOBAL__ ActContext *c) {

    /* buffer swap */

    if (agent->b_buffer_count != 0) {
        agent->f_buffer = float4_div_scalar(agent->b_buffer, (float)agent->b_buffer_count);
        agent->b_buffer = float4_null();
        agent->b_buffer_count = 0;
    }

    /* move */

    agent->p = float4_add(agent->p, agent->v);

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
