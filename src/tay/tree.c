#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <float.h>
#include <assert.h>

#define NODE_DIM_UNDECIDED  100
#define NODE_DIM_LEAF       101
#define AGENT_POSITION(__agent__) ((float *)(__agent__ + 1))


typedef struct {
    float min[TAY_MAX_DIMENSIONS];
    float max[TAY_MAX_DIMENSIONS];
} Box;

static void _reset_box(Box *box, int dims) {
    for (int i = 0; i < dims; ++i) {
        box->min[i] = FLT_MAX;
        box->max[i] = -FLT_MAX;
    }
}

static void _update_box(Box *box, float *p, int dims) {
    for (int i = 0; i < dims; ++i) {
        if (p[i] < box->min[i])
            box->min[i] = p[i];
        if (p[i] > box->max[i])
            box->max[i] = p[i];
    }
}

typedef struct Node {
    struct Node *lo, *hi; /* child partitions */
    TayAgent *first[TAY_MAX_GROUPS]; /* agents contained in this node (fork or leaf) */
    int dim; /* dimension along which the node's partition is divided into child partitions */
    Box box;
} Node;

static void _clear_node(Node *node) {
    node->lo = 0;
    node->hi = 0;
    node->dim = NODE_DIM_UNDECIDED;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i)
        node->first[i] = 0;
}

typedef struct {
    TayAgent *first[TAY_MAX_GROUPS]; /* agents are kept here while not in nodes */
    Node *nodes; /* nodes storage, first node is always the root node */
    int nodes_cap;
    int available_node;
    int dimensions;
    float diameters[TAY_MAX_DIMENSIONS];
    Box box;
} Tree;

static Tree *_init(int dimensions, float *diameters) {
    Tree *t = calloc(1, sizeof(Tree));
    t->dimensions = dimensions;
    t->nodes_cap = 10000;
    t->nodes = malloc(t->nodes_cap * sizeof(Node));
    t->available_node = 1; /* after root which is always allocated */
    _reset_box(&t->box, t->dimensions);
    for (int i = 0; i < dimensions; ++i)
        t->diameters[i] = diameters[i];
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
    _update_box(&tree->box, AGENT_POSITION(agent), tree->dimensions);
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
            _update_box(&tree->box, AGENT_POSITION(a), tree->dimensions);
            if (a->next)
                a = a->next;
            else
                break;
        }

        /* add all agents to global list */
        a->next = tree->first[i];
        tree->first[i] = node->first[i];
    }

    _move_agents_from_node_to_tree(tree, node->lo);
    _move_agents_from_node_to_tree(tree, node->hi);
}

static int _get_partition_dimension(float *min, float *max, float *diameters, int dimensions) {
    float max_r = 2.0f; /* partition side and cell side ratio must be > 2.0 for partition to occur */
    int max_d = NODE_DIM_LEAF;
    for (int i = 0; i < dimensions; ++i) {
        float r = (max[i] - min[i]) / diameters[i];
        if (r > max_r) {
            max_r = r;
            max_d = i;
        }
    }
    return max_d;
}

static void _sort_agent(Tree *tree, Node *node, TayAgent *agent, int group) {
    if (node->dim == NODE_DIM_UNDECIDED) {
        node->dim = _get_partition_dimension(node->box.min, node->box.max, tree->diameters, tree->dimensions);
        if (node->dim != NODE_DIM_LEAF) {
            node->lo = tree->nodes + tree->available_node++;
            node->hi = tree->nodes + tree->available_node++;
            _clear_node(node->lo);
            _clear_node(node->hi);
        }
    }

    if (node->dim == NODE_DIM_LEAF) {
        agent->next = node->first[group];
        node->first[group] = agent;
    }
    else {
        float mid = (node->box.max[node->dim] + node->box.min[node->dim]) * 0.5f;
        float pos = AGENT_POSITION(agent)[node->dim];
        if (pos < mid) {
            node->lo->box = node->box;
            node->lo->box.max[node->dim] = mid;
            _sort_agent(tree, node->lo, agent, group);
        }
        else {
            node->hi->box = node->box;
            node->hi->box.min[node->dim] = mid;
            _sort_agent(tree, node->hi, agent, group);
        }
    }
}

static void _update(TaySpace *space) {
    Tree *t = space->storage;
    _move_agents_from_node_to_tree(t, t->nodes);
    t->available_node = 1;
    _clear_node(t->nodes);
    t->nodes->box = t->box;

    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayAgent *next = t->first[i];
        while (next) {
            TayAgent *a = next;
            next = next->next;
            _sort_agent(t, t->nodes, a, i);
        }
    }

    _reset_box(&t->box, t->dimensions);
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

void tree_init(TaySpace *space, int dimensions, float *diameters) {
    space_init(space, _init(dimensions, diameters), _destroy, _add, _perception, _action, _post, _iter, _update);
}
