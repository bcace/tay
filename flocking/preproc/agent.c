void agent_see(__GLOBAL__ Agent *a, __GLOBAL__ Agent *b, __GLOBAL__ SeeContext *c) {
}

void agent_act(__GLOBAL__ Agent *a, __GLOBAL__ ActContext *c) {
    a->p = float4_add(a->p, a->v);
}
