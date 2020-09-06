#include "state.h"
#include "tay.h"
#include "thread.h"
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>

#define NODE_DIM_UNDECIDED  100
#define NODE_DIM_LEAF       101

#define TREE_THREADED 1


const float radius_to_cell_size_ratio = 0.4f;

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
    TayAgent *seer_agents;
    TayAgent **seen_bundles;
    int seen_bundles_count;
} SeerTask;

static void _init_multi_see_context(SeerTask *c, TayAgent *seer_agents, TayAgent **seen_bundles) {
    c->seer_agents = seer_agents;
    c->seen_bundles = seen_bundles;
    c->seen_bundles_count = 0;
}

typedef struct {
    void *context;
    TayPass *pass;
    SeerTask *first;
    int count;
    int dims;
} MultiSeerTask;

static void _init_multi_seer_task(MultiSeerTask *t, TayPass *pass, SeerTask *first, int count, void *context, int dims) {
    t->pass = pass;
    t->first = first;
    t->count = count;
    t->context = context;
    t->dims = dims;
}

static void _multi_see_func(MultiSeerTask *see_contexts) {
    for (int i = 0; i < see_contexts->count; ++i) {
        SeerTask *c = see_contexts->first + i;

        TAY_SEE_FUNC func = see_contexts->pass->see;
        float *radii = see_contexts->pass->radii;

        for (TayAgent *a = c->seer_agents; a; a = a->next) {
            float *pa = TAY_AGENT_POSITION(a);

            for (int j = 0; j < c->seen_bundles_count; ++j) {

                for (TayAgent *b = c->seen_bundles[j]; b; b = b->next) {
                    float *pb = TAY_AGENT_POSITION(b);

                    if (a == b) /* this can be removed for cases where beg_a != beg_b */
                        continue;

                    for (int k = 0; k < see_contexts->dims; ++k) {
                        float d = pa[k] - pb[k];
                        if (d < -radii[k] || d > radii[k])
                            goto OUTSIDE_RADII;
                    }

                    func(a, b, see_contexts->context);

                    OUTSIDE_RADII:;
                }
            }
        }
    }
}

typedef struct {
    TayAgent *first[TAY_MAX_GROUPS]; /* agents are kept here while not in nodes */
    Node *nodes; /* nodes storage, first node is always the root node */
    int nodes_cap;
    int available_node;
    int dims;
    float diameters[TAY_MAX_DIMENSIONS];
    Box box;
#if TREE_THREADED
    SeerTask *seer_tasks;
    int seer_tasks_count;
    TayAgent **seen_bundles; /* storage for see task bundles */
    int seen_bundles_count;
#endif
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
#if TREE_THREADED
    t->seer_tasks = malloc(t->nodes_cap * sizeof(SeerTask));
    t->seer_tasks_count = 0;
    t->seen_bundles = malloc(t->nodes_cap * sizeof(TayAgent *) * 8);
    t->seen_bundles_count = 0;
#endif
    return t;
}

static void _destroy(TaySpace *space) {
    Tree *t = space->storage;
    free(t->nodes);
#if TREE_THREADED
    free(t->seer_tasks);
    free(t->seen_bundles);
#endif
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
    void *context;
    TayPass *pass;
    TayAgent *seer_agents;
    TayAgent *seen_agents;
    int dims;
} SingleSeeContext;

static void _init_single_see_context(SingleSeeContext *c, TayPass *pass, TayAgent *seer_agents, TayAgent *seen_agents, void *context, int dims) {
    c->context = context;
    c->pass = pass;
    c->seer_agents = seer_agents;
    c->seen_agents = seen_agents;
    c->dims = dims;
}

static void _single_see_func(SingleSeeContext *c) {
    TAY_SEE_FUNC func = c->pass->see;
    float *radii = c->pass->radii;

    for (TayAgent *a = c->seer_agents; a; a = a->next) {
        float *pa = TAY_AGENT_POSITION(a);

        for (TayAgent *b = c->seen_agents; b; b = b->next) {
            float *pb = TAY_AGENT_POSITION(b);

            if (a == b) /* this can be removed for cases where beg_a != beg_b */
                continue;

            for (int k = 0; k < c->dims; ++k) {
                float d = pa[k] - pb[k];
                if (d < -radii[k] || d > radii[k])
                    goto OUTSIDE_RADII;
            }

            func(a, b, c->context);

            OUTSIDE_RADII:;
        }
    }
}

static void _traverse_seen_neighbors(Tree *tree, Node *seer_node, Node *seen_node, TayPass *pass, Box seer_box, void *context) {
    for (int i = 0; i < tree->dims; ++i)
        if (seer_box.min[i] > seen_node->box.max[i] || seer_box.max[i] < seen_node->box.min[i])
            return;
    if (seen_node->first[pass->seen_group]) { /* if there are any "seen" agents */
#if TREE_THREADED
        SeerTask *task = tree->seer_tasks + tree->seer_tasks_count;
        task->seen_bundles[task->seen_bundles_count++] = seen_node->first[pass->seen_group];
#else
        assert(runner.count == 1);
        SingleSeeContext see_context;
        _init_single_see_context(&see_context, pass, seer_node->first[pass->seer_group], seen_node->first[pass->seen_group], context, tree->dims);
        tay_thread_set_task(0, _single_see_func, &see_context);
        tay_runner_run_no_threads();
#endif
    }
    if (seen_node->lo)
        _traverse_seen_neighbors(tree, seer_node, seen_node->lo, pass, seer_box, context);
    if (seen_node->hi)
        _traverse_seen_neighbors(tree, seer_node, seen_node->hi, pass, seer_box, context);
}

static void _traverse_seers(Tree *tree, Node *node, TayPass *pass, void *context) {
    if (node->first[pass->seer_group]) { /* if there are any "seeing" agents */
        Box seer_box = node->box;
        for (int i = 0; i < tree->dims; ++i) {
            seer_box.min[i] -= pass->radii[i];
            seer_box.max[i] += pass->radii[i];
        }

#if TREE_THREADED
        SeerTask *task = tree->seer_tasks + tree->seer_tasks_count;
        _init_multi_see_context(task, node->first[pass->seer_group], tree->seen_bundles + tree->seen_bundles_count);
#endif
        _traverse_seen_neighbors(tree, node, tree->nodes, pass, seer_box, context);
#if TREE_THREADED
        if (task->seen_bundles_count) {
            tree->seen_bundles_count += task->seen_bundles_count;
            assert(tree->seen_bundles_count < tree->nodes_cap * 8);
            ++tree->seer_tasks_count;
        }
#endif
    }
    if (node->lo)
        _traverse_seers(tree, node->lo, pass, context);
    if (node->hi)
        _traverse_seers(tree, node->hi, pass, context);
}

static void _see(TaySpace *space, TayPass *pass, void *context) {
    Tree *t = space->storage;
#if TREE_THREADED
    /* collect thread tasks */
    t->seer_tasks_count = 0;
    t->seen_bundles_count = 0;
#endif
    _traverse_seers(t, t->nodes, pass, context);
#if TREE_THREADED
    /* distribute collected thread tasks among available threads */

    assert(t->seer_tasks_count >= runner.count);

    int fill = (int)floor(t->seer_tasks_count / (double)runner.count);
    int rest = t->seer_tasks_count - (runner.count - 1) * fill;

    static MultiSeerTask multi_seer_tasks[TAY_MAX_THREADS];

    int i = 0;
    for (; i < runner.count - 1; ++i) {
        MultiSeerTask *multi_seer_task = multi_seer_tasks + i;
        _init_multi_seer_task(multi_seer_task, pass, t->seer_tasks + i * fill, fill, context, t->dims);
        tay_thread_set_task(i, _multi_see_func, multi_seer_task);
    }
    {
        MultiSeerTask *see_contexts = multi_seer_tasks + i;
        _init_multi_seer_task(see_contexts, pass, t->seer_tasks + i * fill, rest, context, t->dims);
        tay_thread_set_task(i, _multi_see_func, see_contexts);
    }
    tay_runner_run();
#endif
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
#if TREE_THREADED
    TayActContext act_context;
    _init_act_context(&act_context, pass, node->first[pass->act_group], context);
    tay_thread_set_task(0, _act_func, &act_context);
    for (int i = 1; i < runner.count; ++i)
        tay_thread_set_task(i, _dummy_act_func, 0);
    tay_runner_run_no_threads();
#else
    assert(runner.count == 1);
    TayActContext act_context;
    _init_act_context(&act_context, pass, node->first[pass->act_group], context);
    tay_thread_set_task(0, _act_func, &act_context);
    tay_runner_run_no_threads();
#endif
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
