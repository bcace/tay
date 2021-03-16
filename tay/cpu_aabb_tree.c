#include "space.h"
#include "thread.h"


typedef struct TreeNode {
    union {
        struct TreeNode *l;
        TayAgentTag *agent;
    };
    struct TreeNode *r;
    Box box;
} TreeNode;

static inline int _is_leaf_node(TreeNode *node) {
    return node->r == 0;
}

static inline float _min(float a, float b) {
    return (a < b) ? a : b;
}

static inline float _max(float a, float b) {
    return (a > b) ? a : b;
}

static inline float4 _float4_min(float4 a, float4 b) {
    return (float4) { _min(a.x, b.x), _min(a.y, b.y), _min(a.z, b.z), _min(a.z, b.z) };
}

static inline float4 _float4_max(float4 a, float4 b) {
    return (float4) { _max(a.x, b.x), _max(a.y, b.y), _max(a.z, b.z), _max(a.z, b.z) };
}

static inline Box _box_union(Box a, Box b) {
    return (Box) { _float4_min(a.min, b.min), _float4_max(a.max, b.max) };
}

static inline Box _box_from_agent(TayAgentTag *agent) {
    return (Box) { float4_agent_min(agent), float4_agent_max(agent) };
}

static void _init_leaf_node(TreeNode *node, TayAgentTag *agent) {
    node->agent = agent;
    node->r = 0;
    node->box.min = float4_agent_min(agent);
    node->box.max = float4_agent_max(agent);
}

static void _init_branch_node(TreeNode *node, TreeNode *a, TreeNode *b) {
    node->l = a;
    node->r = b;
    node->box.min = _float4_min(a->box.min, b->box.min);
    node->box.max = _float4_max(a->box.max, b->box.max);
}

static float _increase_in_volume(Box box, Box target_box, int dims) {
    float target_vol = 1.0f;
    float union_vol = 1.0f;
    for (int i = 0; i < dims; ++i) {
        target_vol *= target_box.max.arr[i] - target_box.min.arr[i];
        union_vol *= _max(target_box.max.arr[i], box.max.arr[i]) - _min(target_box.min.arr[i], box.min.arr[i]);
    }
    return union_vol - target_vol;
}

static TreeNode *_find_best_leaf_node(TreeNode *node, Box box, int dims, TreeNode **parent) {
    if (_is_leaf_node(node))
        return node;
    *parent = node;
    if (_increase_in_volume(box, node->l->box, dims) < _increase_in_volume(box, node->r->box, dims))
        return _find_best_leaf_node(node->l, box, dims, parent);
    else
        return _find_best_leaf_node(node->r, box, dims, parent);
}

static int _leaf_node_should_grow(Box box, float4 radii, int dims) {
    for (int i = 0; i < dims; ++i)
        if (box.max.arr[i] - box.min.arr[i] < radii.arr[i])
            return 1;
    return 0;
}

void cpu_aabb_tree_sort(Space *space, TayGroup *groups) {
    CpuAABBTree *tree = &space->cpu_aabb_tree;

    tree->nodes = space->shared;
    tree->nodes_count = 0;
    tree->max_nodes = space->shared_size / (int)sizeof(TreeNode);
    tree->root = 0;

    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = groups + group_i;

        if (group->space != space) /* sort only agents belonging to this space */
            continue;

        TayAgentTag *next = space->first[group_i];
        while (next) {
            TayAgentTag *agent = next;
            next = next->next;

            if (tree->root == 0) { /* first node */
                TreeNode *node = tree->nodes + tree->nodes_count++;
                _init_leaf_node(node, agent);
                tree->root = node;
            }
            else { /* first node already exists */
                Box agent_box = _box_from_agent(agent);

                TreeNode *parent = 0;
                TreeNode *leaf = _find_best_leaf_node(tree->root, agent_box, space->dims, &parent);

                /* if leaf node's box is smaller than suggested partition size (space->radii) or there's
                no more space for new nodes just add agent to the leaf node and expand the leaf node's box */
                if (tree->nodes_count == tree->max_nodes || _leaf_node_should_grow(leaf->box, space->radii, space->dims)) {
                    agent->next = leaf->agent;
                    leaf->agent = agent;
                    leaf->box = _box_union(leaf->box, agent_box);
                }
                /* create new node for agent, put both new node and leaf into a new parent node and
                replace the leaf node with the new parent node in the old parent node */
                else {
                    TreeNode *new_node = tree->nodes + tree->nodes_count++;
                    TreeNode *new_parent = tree->nodes + tree->nodes_count++;

                    _init_leaf_node(new_node, agent);
                    _init_branch_node(new_parent, leaf, new_node);

                    if (parent == 0) /* only root node found */
                        tree->root = new_parent;
                    else {
                        if (leaf == parent->l)
                            parent->l = new_parent;
                        else
                            parent->r = new_parent;
                    }
                }
            }
        }

        space->first[group_i] = 0;
        space->counts[group_i] = 0;
    }
}

void cpu_aabb_tree_unsort(Space *space, TayGroup *groups) {
    CpuAABBTree *tree = &space->cpu_aabb_tree;

    box_reset(&space->box, space->dims);

    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = groups + group_i;

        if (group->space != space) /* only groups belonging to this space */
            continue;

        for (int node_i = 0; node_i < tree->nodes_count; ++node_i) {
            TreeNode *node = tree->nodes + node_i;

            if (_is_leaf_node(node))
                space_return_agents(space, group_i, node->agent, group->is_point);
        }
    }
}

typedef struct {
    TayPass *pass;
    int counter;
} ActTask;

static void _init_act_task(ActTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->counter = thread_i;
}

static void _act_func(ActTask *task, TayThreadContext *thread_context) {
    CpuAABBTree *tree = &task->pass->act_space->cpu_aabb_tree;

    for (int node_i = 0; node_i < tree->nodes_count; ++node_i) {
        TreeNode *node = tree->nodes + node_i;

        if (_is_leaf_node(node)) {
            if (task->counter % runner.count == 0)
                for (TayAgentTag *tag = node->agent; tag; tag = tag->next)
                    task->pass->act(tag, thread_context->context);
            ++task->counter;
        }
    }
}

void cpu_aabb_tree_act(TayPass *pass) {
    static ActTask tasks[TAY_MAX_THREADS];

    CpuAABBTree *tree = &pass->act_space->cpu_aabb_tree;

    for (int thread_i = 0; thread_i < runner.count; ++thread_i) {
        ActTask *task = tasks + thread_i;
        _init_act_task(task, pass, thread_i);
        tay_thread_set_task(thread_i, _act_func, task, pass->context);
    }

    tay_runner_run();
}

typedef struct {
    TayPass *pass;
    int counter;
} SeeTask;

static void _init_see_task(SeeTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->counter = thread_i;
}

// TODO: this one is only for single space passes!!!
static void _see_func(SeeTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    Space *space = pass->seer_space;
    CpuAABBTree *tree = &space->cpu_aabb_tree;

    for (int seer_node_i = 0; seer_node_i < tree->nodes_count; ++seer_node_i) {
        TreeNode *seer_node = tree->nodes + seer_node_i;

        if (_is_leaf_node(seer_node)) {
            if (task->counter % runner.count == 0) {

                for (int seen_node_i = 0; seen_node_i < tree->nodes_count; ++seen_node_i) {
                    TreeNode *seen_node = tree->nodes + seen_node_i;

                    pass->pairing_func(seer_node->agent, seen_node->agent, pass->see, pass->radii, space->dims, thread_context);
                }
            }
            ++task->counter;
        }
    }
}

void cpu_aabb_tree_see(TayPass *pass) {
    static SeeTask tasks[TAY_MAX_THREADS];

    CpuAABBTree *tree = &pass->act_space->cpu_aabb_tree;

    if (pass->seer_space == pass->seen_space) {

        for (int thread_i = 0; thread_i < runner.count; ++thread_i) {
            SeeTask *task = tasks + thread_i;
            _init_see_task(task, pass, thread_i);
            tay_thread_set_task(thread_i, _see_func, task, pass->context);
        }
    }

    tay_runner_run();
}