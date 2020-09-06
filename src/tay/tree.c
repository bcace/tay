#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#define NODE_DIM_UNDECIDED  100
#define NODE_DIM_LEAF       101

#define TREE_THREADED 0


const float radius_to_cell_size_ratio = 1.0f;

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
    int dims;
    float diameters[TAY_MAX_DIMENSIONS];
    Box box;
} Tree;

static Tree *_init(int dims, float *radii) {
    Tree *t = calloc(1, sizeof(Tree));
    t->dims = dims;
    t->nodes_cap = 100000;
    t->nodes = malloc(t->nodes_cap * sizeof(Node));
    t->available_node = 1; /* root node is always allocated */
    _clear_node(t->nodes);
    _reset_box(&t->box, t->dims);
    for (int i = 0; i < dims; ++i)
        t->diameters[i] = radii[i] * radius_to_cell_size_ratio;
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
    _update_box(&tree->box, TAY_AGENT_POSITION(agent), tree->dims);
}

static void _add(TaySpace *space, TayAgent *agent, int group) {
    Tree *t = space->storage;
    _add_to_non_sorted(t, agent, group);
}

static void _move_agents_from_node_to_tree(Tree *tree, Node *node) {
    if (node == 0)
        return;
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        if (node->first[i] == 0)
            continue;
        TayAgent *last = node->first[i];
        /* update global extents and find the last agent */
        while (1) {
            _update_box(&tree->box, TAY_AGENT_POSITION(last), tree->dims);
            if (last->next)
                last = last->next;
            else
                break;
        }
        /* add all agents to global list */
        last->next = tree->first[i];
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
    if (node->dim == NODE_DIM_UNDECIDED)
        node->dim = _get_partition_dimension(node->box.min, node->box.max, tree->diameters, tree->dims);

    if (node->dim == NODE_DIM_LEAF) {
        agent->next = node->first[group];
        node->first[group] = agent;
    }
    else {
        float mid = (node->box.max[node->dim] + node->box.min[node->dim]) * 0.5f;
        float pos = TAY_AGENT_POSITION(agent)[node->dim];
        if (pos < mid) {
            if (node->lo == 0) {
                assert(tree->available_node < tree->nodes_cap);
                node->lo = tree->nodes + tree->available_node++;
                _clear_node(node->lo);
            }
            node->lo->box = node->box;
            node->lo->box.max[node->dim] = mid;
            _sort_agent(tree, node->lo, agent, group);
        }
        else {
            if (node->hi == 0) {
                assert(tree->available_node < tree->nodes_cap);
                node->hi = tree->nodes + tree->available_node++;
                _clear_node(node->hi);
            }
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

    /* sort all agents from tree into nodes */
    for (int i = 0; i < TAY_MAX_GROUPS; ++i) {
        TayAgent *next = t->first[i];
        while (next) {
            TayAgent *a = next;
            next = next->next;
            _sort_agent(t, t->nodes, a, i);
        }
        t->first[i] = 0;
    }

    _reset_box(&t->box, t->dims);
}

typedef struct {
    Tree *tree;
    TayPass *pass;
    void *context;
    int counter;
    int dims;
} ThreadSeeTask;

static void _init_thread_see_task(ThreadSeeTask *t, Tree *tree, TayPass *pass, void *context, int thread_index) {
    t->tree = tree;
    t->pass = pass;
    t->context = context;
    t->counter = thread_index;
}

static void _thread_traverse_seen(Tree *tree, Node *seer_node, Node *seen_node, TayPass *pass, Box seer_box, void *context) {
    for (int i = 0; i < tree->dims; ++i)
        if (seer_box.min[i] > seen_node->box.max[i] || seer_box.max[i] < seen_node->box.min[i])
            return;
    if (seen_node->first[pass->seen_group]) { /* if there are any "seen" agents */
        TAY_SEE_FUNC func = pass->see;
        float *radii = pass->radii;

        TayAgent *seer_agents = seer_node->first[pass->seer_group];
        TayAgent *seen_agents = seen_node->first[pass->seen_group];

        for (TayAgent *a = seer_agents; a; a = a->next) {
            float *pa = TAY_AGENT_POSITION(a);

            for (TayAgent *b = seen_agents; b; b = b->next) {
                float *pb = TAY_AGENT_POSITION(b);

                if (a == b) /* this can be removed for cases where beg_a != beg_b */
                    continue;

                for (int k = 0; k < tree->dims; ++k) {
                    float d = pa[k] - pb[k];
                    if (d < -radii[k] || d > radii[k])
                        goto OUTSIDE_RADII;
                }

                func(a, b, context);

                OUTSIDE_RADII:;
            }
        }
    }
    if (seen_node->lo)
        _thread_traverse_seen(tree, seer_node, seen_node->lo, pass, seer_box, context);
    if (seen_node->hi)
        _thread_traverse_seen(tree, seer_node, seen_node->hi, pass, seer_box, context);
}

static void _thread_traverse_seers(ThreadSeeTask *task, Node *node) {
    if (task->counter % runner.count == 0) {
        if (node->first[task->pass->seer_group]) { /* if there are any "seeing" agents */
            Box seer_box = node->box;
            for (int i = 0; i < task->tree->dims; ++i) {
                seer_box.min[i] -= task->pass->radii[i];
                seer_box.max[i] += task->pass->radii[i];
            }
            _thread_traverse_seen(task->tree, node, task->tree->nodes, task->pass, seer_box, task->context);
        }
    }
    ++task->counter;
    if (node->lo)
        _thread_traverse_seers(task, node->lo);
    if (node->hi)
        _thread_traverse_seers(task, node->hi);
}

static void _thread_see_traverse(ThreadSeeTask *task) {
    _thread_traverse_seers(task, task->tree->nodes);
}

static void _see(TaySpace *space, TayPass *pass, void *context) {
    Tree *t = space->storage;

    static ThreadSeeTask tasks[TAY_MAX_THREADS];

    for (int i = 0; i < runner.count; ++i) {
        ThreadSeeTask *task = tasks + i;
        _init_thread_see_task(task, t, pass, context, i);
        tay_thread_set_task(i, _thread_see_traverse, task);
    }

    tay_runner_run();
}

typedef struct {
    void *context;
    TayPass *pass;
    TayAgent *agents;
} TayActContext;

static void _init_act_context(TayActContext *c, TayPass *pass, TayAgent *agents, void *context) {
    c->context = context;
    c->pass = pass;
    c->agents = agents;
}

static void _act_func(TayActContext *c) {
    for (TayAgent *a = c->agents; a; a = a->next)
        c->pass->act(a, c->context);
}

static void _dummy_act_func(void *c) {
}

static void _traverse_actors(Node *node, TayPass *pass, void *context) {
    TayActContext act_context;
    _init_act_context(&act_context, pass, node->first[pass->act_group], context);
    tay_thread_set_task(0, _act_func, &act_context);
    for (int i = 1; i < runner.count; ++i)
        tay_thread_set_task(i, _dummy_act_func, 0);
    tay_runner_run_no_threads();
    if (node->lo)
        _traverse_actors(node->lo, pass, context);
    if (node->hi)
        _traverse_actors(node->hi, pass, context);
}

static void _act(TaySpace *space, TayPass *pass, void *context) {
    Tree *t = space->storage;
    _traverse_actors(t->nodes, pass, context);
}

static void _post(TaySpace *space, void (*func)(void *), void *context) {
    Tree *t = space->storage;
    assert(0); /* not implemented */
}

static void _iter(TaySpace *space, int group, void (*func)(void *, void *), void *context) {
    Tree *t = space->storage;
    // assert(0); /* not implemented */
}

void tree_init(TaySpace *space, int dims, float *radii) {
    space_init(space, _init(dims, radii), dims, _destroy, _add, _see, _act, _post, _iter, _update);
}
