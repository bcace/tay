#ifndef tay_state_h
#define tay_state_h

#define TAY_MAX_GROUPS 32
#define TAY_MAX_PASSES 32


typedef struct TayGroup {
    void *storage;      /* agents storage */
    struct TayAgent *first;    /* single linked list of available agents from storage */
    int size;           /* agent size in bytes */
    int capacity;       /* max. number of agents */
} TayGroup;

typedef struct TayPass {
    int group1, group2;
    void (*perception)(void *, void *, void *);
    void (*action)(void *, void *);
} TayPass;

typedef struct TaySpace {
    void *storage;
    void (*destroy)(struct TaySpace *space);
    void (*add)(struct TaySpace *space, struct TayAgent *agent, int group);
    void (*perception)(struct TaySpace *space, int group1, int group2, void (*func)(void *, void *, void *), void *context);
    void (*action)(struct TaySpace *space, int group, void (*func)(void *, void *), void *context);
} TaySpace;

typedef struct TayState {
    TayGroup groups[TAY_MAX_GROUPS];
    TayPass passes[TAY_MAX_PASSES];
    int passes_count;
    TaySpace space;
} TayState;

void space_plain_init(TaySpace *space);

#endif
