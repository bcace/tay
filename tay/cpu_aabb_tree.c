#include "space.h"
#include "thread.h"


typedef struct TreeNode {
    union {
        struct TreeNode *a;
        TayAgentTag *first;
    };
    struct TreeNode *b;
    struct TreeNode *parent;
    Box box;
    float volume;
} TreeNode;

static inline int _is_leaf_node(TreeNode *node) {
    return node->b == 0;
}

static inline float _min(float a, float b) {
    return (a < b) ? a : b;
}

static inline float _max(float a, float b) {
    return (a > b) ? a : b;
}

static inline float4 _float4_min(float4 a, float4 b) {
    return (float4){ _min(a.x, b.x), _min(a.y, b.y), _min(a.z, b.z), _min(a.z, b.z) };
}

static inline float4 _float4_max(float4 a, float4 b) {
    return (float4){ _max(a.x, b.x), _max(a.y, b.y), _max(a.z, b.z), _max(a.z, b.z) };
}

static inline Box _box_union(Box a, Box b) {
    return (Box){ _float4_min(a.min, b.min), _float4_max(a.max, b.max) };
}

static inline float _box_volume(Box box, int dims) {
    float v = 1.0f;
    for (int i = 0; i < dims; ++i)
        v *= box.max.arr[i] - box.min.arr[i];
    return v;
}

static void _init_leaf_node(TreeNode *node, TayAgentTag *agent, int dims) {
    agent->next = 0;
    node->first = agent;
    node->b = 0;
    node->box.min = float4_agent_min(agent);
    node->box.max = float4_agent_max(agent);
    node->volume = _box_volume(node->box, dims);
    node->parent = 0;
}

static void _init_branch_node(TreeNode *node, TreeNode *a, TreeNode *b, int dims) {
    node->a = a;
    node->b = b;
    node->box.min = _float4_min(a->box.min, b->box.min);
    node->box.max = _float4_max(a->box.max, b->box.max);
    node->volume = _box_volume(node->box, dims);
    node->parent = 0;
    a->parent = node;
    b->parent = node;
}

static void _update_node_box(TreeNode *node, int dims) {
    node->box = _box_union(node->a->box, node->b->box);
    node->volume = _box_volume(node->box, dims);
}

static float _increase_in_volume(Box box, TreeNode *node, int dims) {
    float union_vol = 1.0f;
    for (int i = 0; i < dims; ++i)
        union_vol *= _max(node->box.max.arr[i], box.max.arr[i]) - _min(node->box.min.arr[i], box.min.arr[i]);
    return union_vol - node->volume;
}

static TreeNode *_find_best_leaf_node(TreeNode *node, Box box, int dims) {
    if (_is_leaf_node(node))
        return node;
    if (_increase_in_volume(box, node->a, dims) < _increase_in_volume(box, node->b, dims))
        return _find_best_leaf_node(node->a, box, dims);
    else
        return _find_best_leaf_node(node->b, box, dims);
}

static int _leaf_node_should_expand(Box agent_box, Box leaf_box, float4 part_sizes, int dims) {
    for (int i = 0; i < dims; ++i)
        if (_max(leaf_box.max.arr[i], agent_box.max.arr[i]) - _min(leaf_box.min.arr[i], agent_box.min.arr[i]) > part_sizes.arr[i])
            return 0;
    return 1;
}

void cpu_aabb_tree_sort(TayGroup *group) {
    Space *space = &group->space;
    CpuAabbTree *tree = &space->cpu_aabb_tree;

    tree->nodes = space->shared;
    tree->nodes_count = 0;
    tree->max_nodes = space->shared_size / (int)sizeof(TreeNode);
    tree->root = 0;

    float4 part_sizes = { space->radii.x * 2.0f, space->radii.y * 2.0f, space->radii.z * 2.0f, space->radii.w * 2.0f };

    TayAgentTag *next = space->first;
    while (next) {
        TayAgentTag *agent = next;
        next = next->next;

        if (tree->root == 0) { /* first node */
            TreeNode *node = tree->nodes + tree->nodes_count++;
            _init_leaf_node(node, agent, space->dims);
            tree->root = node;
        }
        else { /* first node already exists */
            Box agent_box = (Box){ float4_agent_min(agent), float4_agent_max(agent) };

            TreeNode *leaf = _find_best_leaf_node(tree->root, agent_box, space->dims);
            TreeNode *old_parent = leaf->parent;
            TreeNode *old_parent_parent = old_parent ? old_parent->parent : 0;

            /* if leaf node's box is smaller than suggested partition size (part_sizes) or there's
            no more space for new nodes just add agent to the leaf node and expand the leaf node's box */
            if (tree->nodes_count == tree->max_nodes || _leaf_node_should_expand(agent_box, leaf->box, part_sizes, space->dims)) {
                agent->next = leaf->first;
                leaf->first = agent;
                leaf->box = _box_union(leaf->box, agent_box);
                leaf->volume = _box_volume(leaf->box, space->dims);
            }
            /* create new node for agent, put both new node and leaf into a new parent node and
            replace the leaf node with the new parent node in the old parent node */
            else {
                TreeNode *new_node = tree->nodes + tree->nodes_count++;
                TreeNode *new_parent = tree->nodes + tree->nodes_count++;

                _init_leaf_node(new_node, agent, space->dims);

                if (old_parent == 0) { /* only root node found */
                    _init_branch_node(new_parent, leaf, new_node, space->dims);

                    tree->root = new_parent;
                }
                else { /* substitute new_parent for leaf in old_parent */
                    float maybe_new_parent_volume = _box_volume(_box_union(leaf->box, new_node->box), space->dims);

                    if (maybe_new_parent_volume < old_parent->volume) {
                        _init_branch_node(new_parent, leaf, new_node, space->dims);
                        
                        if (leaf == old_parent->a)
                            old_parent->a = new_parent;
                        else
                            old_parent->b = new_parent;
                        new_parent->parent = old_parent;
                    }
                    else {
                        _init_branch_node(new_parent, old_parent, new_node, space->dims);

                        if (old_parent_parent == 0)
                            tree->root = new_parent;
                        else {
                            if (old_parent == old_parent_parent->a)
                                old_parent_parent->a = new_parent;
                            else
                                old_parent_parent->b = new_parent;
                            new_parent->parent = old_parent_parent;   
                        }
                    }
                }
            }

            for (TreeNode *node = old_parent; node; node = node->parent)
                _update_node_box(node, space->dims);
        }
    }

    space->first = 0;
    space->count = 0;
}

void cpu_aabb_tree_unsort(TayGroup *group) {
    Space *space = &group->space;
    CpuAabbTree *tree = &space->cpu_aabb_tree;

    box_reset(&space->box, space->dims);

    for (unsigned node_i = 0; node_i < tree->nodes_count; ++node_i) {
        TreeNode *node = tree->nodes + node_i;

        if (_is_leaf_node(node))
            space_return_agents(space, node->first, group->is_point);
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
    CpuAabbTree *tree = &task->pass->act_space->cpu_aabb_tree;

    for (unsigned node_i = 0; node_i < tree->nodes_count; ++node_i) {
        TreeNode *node = tree->nodes + node_i;

        if (_is_leaf_node(node)) {
            if (task->counter % runner.count == 0)
                for (TayAgentTag *tag = node->first; tag; tag = tag->next)
                    task->pass->act(tag, thread_context->context);
            ++task->counter;
        }
    }
}

void cpu_aabb_tree_act(TayPass *pass) {
    static ActTask tasks[TAY_MAX_THREADS];

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

static void _node_see_func(TayPass *pass, TayAgentTag *seer_agents, Box seer_box, TreeNode *seen_node, int dims, TayThreadContext *thread_context) {
    for (int i = 0; i < dims; ++i)
        if (seer_box.max.arr[i] < seen_node->box.min.arr[i] || seer_box.min.arr[i] > seen_node->box.max.arr[i])
            return;

    if (_is_leaf_node(seen_node))
        pass->pairing_func(seer_agents, seen_node->first, pass->see, pass->radii, dims, thread_context);
    else {
        _node_see_func(pass, seer_agents, seer_box, seen_node->a, dims, thread_context);
        _node_see_func(pass, seer_agents, seer_box, seen_node->b, dims, thread_context);
    }
}

void cpu_aabb_tree_see_seen(TayPass *pass, TayAgentTag *seer_agents, Box seer_box, int dims, TayThreadContext *thread_context) {
    _node_see_func(pass, seer_agents, seer_box, pass->seen_space->cpu_aabb_tree.root, dims, thread_context);
}

static void _see_func(SeeTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    Space *seer_space = pass->seer_space;
    Space *seen_space = pass->seen_space;
    CpuAabbTree *seer_tree = &seer_space->cpu_aabb_tree;

    int min_dims = (seer_space->dims < seen_space->dims) ? seer_space->dims : seen_space->dims;

    for (unsigned seer_node_i = 0; seer_node_i < seer_tree->nodes_count; ++seer_node_i) {
        TreeNode *seer_node = seer_tree->nodes + seer_node_i;

        if (_is_leaf_node(seer_node)) {
            if (task->counter % runner.count == 0) {

                Box seer_box = seer_node->box;
                for (int i = 0; i < min_dims; ++i) {
                    seer_box.min.arr[i] -= pass->radii.arr[i];
                    seer_box.max.arr[i] += pass->radii.arr[i];
                }

                pass->struct_seen_func(pass, seer_node->first, seer_box, min_dims, thread_context);
            }
            ++task->counter;
        }
    }
}

void cpu_aabb_tree_see(TayPass *pass) {
    static SeeTask tasks[TAY_MAX_THREADS];

    for (int thread_i = 0; thread_i < runner.count; ++thread_i) {
        SeeTask *task = tasks + thread_i;
        _init_see_task(task, pass, thread_i);
        tay_thread_set_task(thread_i, _see_func, task, pass->context);
    }

    tay_runner_run();
}
