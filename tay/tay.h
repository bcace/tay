#ifndef tay_h
#define tay_h

#define float4_agent_position(__agent_tag__) (*(float4 *)((TayAgentTag *)(__agent_tag__) + 1))
#define float3_agent_position(__agent_tag__) (*(float3 *)((TayAgentTag *)(__agent_tag__) + 1))
#define float2_agent_position(__agent_tag__) (*(float2 *)((TayAgentTag *)(__agent_tag__) + 1))

#define float4_agent_min(__agent_tag__) (*(float4 *)((TayAgentTag *)(__agent_tag__) + 1))
#define float3_agent_min(__agent_tag__) (*(float3 *)((TayAgentTag *)(__agent_tag__) + 1))
#define float2_agent_min(__agent_tag__) (*(float2 *)((TayAgentTag *)(__agent_tag__) + 1))
#define float4_agent_max(__agent_tag__) (*(float4 *)((char *)(__agent_tag__) + sizeof(TayAgentTag) + sizeof(float4)))
#define float3_agent_max(__agent_tag__) (*(float3 *)((char *)(__agent_tag__) + sizeof(TayAgentTag) + sizeof(float4)))
#define float2_agent_max(__agent_tag__) (*(float2 *)((char *)(__agent_tag__) + sizeof(TayAgentTag) + sizeof(float4)))

/*
** Enums
*/

typedef enum TaySpaceType {
    ST_NONE                     = 0x00,
    TAY_CPU_SIMPLE              = 0x01,
    TAY_CPU_TREE                = 0x02,
    TAY_CPU_GRID                = 0x04,
    TAY_GPU_SIMPLE_DIRECT       = 0x08,
    TAY_GPU_SIMPLE_INDIRECT     = 0x10,
} TaySpaceType;

typedef enum TayError {
    TAY_ERROR_NONE = 0,
    TAY_ERROR_POINT_NONPOINT_MISMATCH,
    TAY_ERROR_SPACE_INDEX_OUT_OF_RANGE,
    TAY_ERROR_GROUP_INDEX_OUT_OF_RANGE,
    TAY_ERROR_STATE_STATUS,
    TAY_ERROR_NOT_IMPLEMENTED,
} TayError;

/*
** basic structs that have to work on both CPU and GPU
*/

#pragma pack(push, 1)

typedef struct float4 {
    union {
        struct {
            float x, y, z, w;
        };
        float arr[4];
    };
} float4;

typedef struct float3 {
    union {
        struct {
            float x, y, z;
        };
        float arr[3];
    };
} float3;

typedef struct float2 {
    union {
        struct {
            float x, y;
            float arr[2];
        };
    };
} float2;

typedef struct {
    float4 min;
    float4 max;
} Box;

typedef struct TayAgentTag {
    struct TayAgentTag *next;
} TayAgentTag;

#pragma pack(pop)

/*
** library API
*/

typedef struct TayState TayState;

TayState *tay_create_state();
void tay_destroy_state(TayState *state);

TayError tay_get_error(TayState *state);

void tay_add_space(TayState *state, TaySpaceType space_type, int space_dims, float4 part_radii, int depth_correction, int shared_size_in_megabytes);

int tay_add_group(TayState *state, unsigned agent_size, unsigned agent_capacity, int is_point, unsigned space_index);
void tay_add_see(TayState *state, int seer_group, int seen_group, void (*func)(void *, void *, void *), const char *func_name, float4 radii, void *context, int context_size);
void tay_add_act(TayState *state, int act_group, void (*func)(void *, void *), const char *func_name, void *context, int context_size);

void *tay_get_available_agent(TayState *state, int group);
void tay_commit_available_agent(TayState *state, int group);
void *tay_get_agent(TayState *state, int group, int index);

void tay_simulation_start(TayState *state);
int tay_run(TayState *state, int steps);
void tay_simulation_end(TayState *state);

double tay_get_ms_per_step_for_last_run(TayState *state);

#endif
