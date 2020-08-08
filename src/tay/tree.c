#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <float.h>
#include <assert.h>


typedef struct Node {
    struct Node *children[1 << TAY_MAX_DIMENSIONS];
    TayAgent *first[TAY_MAX_GROUPS];
} Node;

typedef struct {
    TayAgent *first[TAY_MAX_GROUPS]; /* agents are kept here while not in nodes */
    Node *nodes, *root;
    int nodes_cap;
    int available_node;
    int dimensions;
    int partitions;
    float min[TAY_MAX_DIMENSIONS];
    float max[TAY_MAX_DIMENSIONS];
} Tree;

static void _reset_extents(float *min, float *max, int dims) {
    for (int i = 0; i < dims; ++i) {
        min[i] = FLT_MAX;
        max[i] = -FLT_MAX;
    }
}

static void _update_extents(float *min, float *max, float *p, int dims) {
    for (int i = 0; i < dims; ++i) {
        if (p[i] < min[i])
            min[i] = p[i];
        if (p[i] > max[i])
            max[i] = p[i];
    }
}

static Tree *_init(int dimensions) {
    Tree *t = calloc(1, sizeof(Tree));
    t->dimensions = dimensions;
    t->partitions = 1 << dimensions;
    t->nodes_cap = 10000;
    t->nodes = calloc(t->nodes_cap, sizeof(Node));
    t->available_node = 0;
    t->root = 0;
    _reset_extents(t->min, t->max, t->dimensions);
    return t;
}

static void _destroy(TaySpace *space) {
    Tree *t = space->storage;
    free(t->nodes);
    free(t);
}

static void _add_to_non_sorted(Tree *tree, TayAgent *agent, int group) {
    agent->next = tree->first[group];
    tree->first[group] = agent;
    _update_extents(tree->min, tree->max, (float *)(agent + 1), tree->dimensions);
}

static void _add(TaySpace *space, TayAgent *agent, int group) {
    Tree *t = space->storage;
    _add_to_non_sorted(t, agent, group);
}

static void _move_agents_from_node_to_tree(Tree *tree, Node *node) {
    if (node == 0)
        return;

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayAgent *a = node->first[i];

        /* update global extents and find the last agent */
        while (1) {
            _update_extents(tree->min, tree->max, (float *)(a + 1), tree->dimensions);
            if (a->next)
                a = a->next;
            else
                break;
        }

        /* add all agents to global list */
        a->next = tree->first[i];
        tree->first[i] = node->first[i];
        node->first[i] = 0;
    }

    for (int i = 0; i < tree->partitions; ++i)
        _move_agents_from_node_to_tree(tree, node->children[i]);
}

static void _update(TaySpace *space) {
    Tree *t = space->storage;
    _move_agents_from_node_to_tree(t, t->root);
    t->root = 0;
    t->available_node = 0;
    // TODO: based on the new extents create the new tree
    // distribute all agents from tree lists into nodes
    _reset_extents(t->min, t->max, t->dimensions);
}

static void _perception(TaySpace *space, int group1, int group2, void (*func)(void *, void *, void *), void *context) {
    Tree *t = space->storage;
    assert(0); /* not implemented */
}

static void _action(TaySpace *space, int group, void(*func)(void *, void *), void *context) {
    Tree *t = space->storage;
    assert(0); /* not implemented */
}

static void _post(TaySpace *space, void (*func)(void *), void *context) {
    Tree *t = space->storage;
    assert(0); /* not implemented */
}

static void _iter(TaySpace *space, int group, void (*func)(void *, void *), void *context) {
    Tree *t = space->storage;
    assert(0); /* not implemented */
}

void tree_init(TaySpace *space, int space_dimensions) {
    space_init(space, _init(space_dimensions), _destroy, _add, _perception, _action, _post, _iter, _update);
}
