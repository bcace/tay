#ifndef tay_state_h
#define tay_state_h

#define TAY_MAX_GROUPS      8
#define TAY_MAX_PASSES      32
#define TAY_MAX_DIMENSIONS  8


typedef struct TayGroup {
    void *storage;          /* agents storage */
    struct TayAgent *first; /* single linked list of available agents from storage */
    int size;               /* agent size in bytes */
    int capacity;           /* max. number of agents */
} TayGroup;

typedef enum TayPassType {
    TAY_PASS_PERCEPTION,
    TAY_PASS_ACTION,
    TAY_PASS_POST,
} TayPassType;

typedef struct TayPass {
    int group1, group2;
    TayPassType type;
    union {
        void (*perception)(void *, void *, void *);
        void (*action)(void *, void *);
        void (*post)(void *);
    };
} TayPass;

typedef struct TaySpace {
    void *storage;
    void (*destroy)(struct TaySpace *space);
    void (*add)(struct TaySpace *space, struct TayAgent *agent, int group);
    void (*perception)(struct TaySpace *space, int group1, int group2, void (*func)(void *, void *, void *), void *context);
    void (*action)(struct TaySpace *space, int group, void (*func)(void *, void *), void *context);
    void (*post)(struct TaySpace *space, void (*func)(void *), void *context);
    void (*iter)(struct TaySpace *space, int group, void (*func)(void *, void *), void *context);
    void (*update)(struct TaySpace *space);
} TaySpace;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    int passes_count;
    TaySpace space;
} TayState;

void space_init(TaySpace *space,
                void *storage,
                void (*destroy)(TaySpace *space),
                void (*add)(TaySpace *space, struct TayAgent *agent, int group),
                void (*perception)(TaySpace *space, int group1, int group2, void (*func)(void *, void *, void *), void *context),
                void (*action)(TaySpace *space, int group, void (*func)(void *, void *), void *context),
                void (*post)(TaySpace *space, void (*func)(void *), void *context),
                void (*iter)(TaySpace *space, int group, void (*func)(void *, void *), void *context),
                void (*update)(TaySpace *space));
void space_simple_init(TaySpace *space);
void tree_init(TaySpace *space, int dimensions, float *diameters);

#endif
