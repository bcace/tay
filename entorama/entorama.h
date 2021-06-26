#ifndef entorama_h
#define entorama_h

#define ENTORAMA_MAX_GROUPS 256


typedef struct EntoramaGroupInfo {
    struct TayGroup *group;
    unsigned max_agents;
} EntoramaGroupInfo;

typedef struct EntoramaSimulationInfo {
    EntoramaGroupInfo groups[ENTORAMA_MAX_GROUPS];
    unsigned groups_count;
} EntoramaSimulationInfo;

typedef int (*ENTORAMA_INIT)(EntoramaSimulationInfo *info, struct TayState *);

typedef struct EntoramaModelInfo {
    ENTORAMA_INIT init;
    float origin_x, origin_y, origin_z;
    float radius;
} EntoramaModelInfo;

typedef int (*ENTORAMA_MAIN)(EntoramaModelInfo *);

#endif
