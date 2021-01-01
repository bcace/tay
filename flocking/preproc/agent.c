void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *c) {
    float3 a_p = float3_get_agent_position(a);
    float3 b_p = float3_get_agent_position(b);

    float dx_sq = b_p.x - a_p.x;
    float dy_sq = b_p.y - a_p.y;
    float dz_sq = b_p.z - a_p.z;
    dx_sq *= dx_sq;
    dy_sq *= dy_sq;
    dz_sq *= dz_sq;
    float d_sq = dx_sq + dy_sq + dz_sq;

    /* separation */

    /* cohesion */

    /* alignment */
}

void agent_act(__GLOBAL__ Agent *a, __GLOBAL__ ActContext *c) {
    float3 p = float3_get_agent_position(a);
    float3_set_agent_position(a, float3_add(p, a->v));
    a->separation.x = 0.0f;
    a->separation.y = 0.0f;
    a->separation.z = 0.0f;
    a->cohesion.x = 0.0f;
    a->cohesion.y = 0.0f;
    a->cohesion.z = 0.0f;
    a->alignment.x = 0.0f;
    a->alignment.y = 0.0f;
    a->alignment.z = 0.0f;
}
