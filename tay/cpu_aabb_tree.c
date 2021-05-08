#include "space.h"
#include "thread.h"
#include <string.h>


typedef struct TreeNode {
    union {
        struct TreeNode *a;
        struct {
            unsigned first_agent_i;
            unsigned count;
        };
    };
    struct TreeNode *b;
    struct TreeNode *parent;
    Box box;
    float volume;
} TreeNode;

static inline int _is_leaf_node(TreeNode *node) {
    return node->b == 0;
}

static inline unsigned _minu(unsigned a, unsigned b) {
    return (a < b) ? a : b;
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

static inline unsigned _node_index(TreeNode *nodes, TreeNode *node) {
    return (unsigned)(node - nodes);
}

static void _init_leaf_node(TreeNode *node, unsigned node_i, TayAgentTag *agent, int dims) {
    agent->part_i = node_i;
    agent->cell_agent_i = 0;
    node->count = 1;
    node->b = 0;
    node->box.min = float4_agent_min(agent);
    node->box.max = float4_agent_max(agent);
    node->volume = _box_volume(node->box, dims);
    node->parent = 0;
}

static void _expand_leaf_node(TreeNode *node, unsigned node_i, TayAgentTag *agent, Box agent_box, int dims) {
    agent->part_i = node_i;
    agent->cell_agent_i = node->count;
    node->count++;
    node->box = _box_union(node->box, agent_box);
    node->volume = _box_volume(node->box, dims);
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

static void _order_nodes(TreeNode *node, unsigned *first_agent_i) {
    if (_is_leaf_node(node)) {
        node->first_agent_i = *first_agent_i;
        *first_agent_i += node->count;
    }
    else {
        _order_nodes(node->a, first_agent_i);
        _order_nodes(node->b, first_agent_i);
    }
}

void cpu_aabb_tree_sort(TayGroup *group) {
    Space *space = &group->space;
    CpuAabbTree *tree = &space->cpu_aabb_tree;

    box_reset(&space->box, space->dims);

    tree->nodes = space->shared;
    tree->nodes_count = 0;
    tree->max_nodes = space->shared_size / (int)sizeof(TreeNode);
    tree->root = 0;

    float4 part_sizes = { space->radii.x * 2.0f, space->radii.y * 2.0f, space->radii.z * 2.0f, space->radii.w * 2.0f };

    for (unsigned agent_i = 0; agent_i < space->count; ++agent_i) {
        TayAgentTag *agent = (TayAgentTag *)(group->storage + group->agent_size * agent_i);

        if (tree->root == 0) { /* first node */
            TreeNode *node = tree->nodes + tree->nodes_count++;
            _init_leaf_node(node, _node_index(tree->nodes, node), agent, space->dims);
            tree->root = node;
        }
        else { /* first node already exists */
            Box agent_box = (Box){ float4_agent_min(agent), float4_agent_max(agent) };

            TreeNode *leaf = _find_best_leaf_node(tree->root, agent_box, space->dims);
            TreeNode *old_parent = leaf->parent;
            TreeNode *old_parent_parent = old_parent ? old_parent->parent : 0;

            /* if leaf node's box is smaller than suggested partition size (part_sizes) or there's
            no more space for new nodes just add agent to the leaf node and expand the leaf node's box */
            if (tree->nodes_count == tree->max_nodes || _leaf_node_should_expand(agent_box, leaf->box, part_sizes, space->dims))
                _expand_leaf_node(leaf, _node_index(tree->nodes, leaf), agent, agent_box, space->dims);
            /* create new node for agent, put both new node and leaf into a new parent node and
            replace the leaf node with the new parent node in the old parent node */
            else {
                TreeNode *new_node = tree->nodes + tree->nodes_count++;
                TreeNode *new_parent = tree->nodes + tree->nodes_count++;

                _init_leaf_node(new_node, _node_index(tree->nodes, new_node), agent, space->dims);

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

    unsigned first_agent_i = 0;
    _order_nodes(tree->root, &first_agent_i);

    for (unsigned i = 0; i < space->count; ++i) {
        TayAgentTag *src = (TayAgentTag *)(group->storage + group->agent_size * i);
        unsigned sorted_agent_i = tree->nodes[src->part_i].first_agent_i + src->cell_agent_i;
        TayAgentTag *dst = (TayAgentTag *)(group->sort_storage + group->agent_size * sorted_agent_i);
        memcpy(dst, src, group->agent_size);
    }

    void *storage = group->storage;
    group->storage = group->sort_storage;
    group->sort_storage = storage;
}

void cpu_aabb_tree_unsort(TayGroup *group) {}

typedef struct {
    TayPass *pass;
    int thread_i;
} SeeTask;

static void _init_see_task(SeeTask *task, TayPass *pass, int thread_i) {
    task->pass = pass;
    task->thread_i = thread_i;
}

static void _node_see_func(TayPass *pass, AgentsSlice seer_slice, Box seer_box, TreeNode *seen_node, int dims, TayThreadContext *thread_context) {
    for (int i = 0; i < dims; ++i)
        if (seer_box.max.arr[i] < seen_node->box.min.arr[i] || seer_box.min.arr[i] > seen_node->box.max.arr[i])
            return;

    if (_is_leaf_node(seen_node)) {
        AgentsSlice seen_slice = {
            pass->seen_group->storage,
            pass->seen_group->agent_size,
            seen_node->first_agent_i,
            seen_node->first_agent_i + seen_node->count
        };
        pass->pairing_func(seer_slice, seen_slice, pass->see, pass->radii, dims, thread_context);
    }
    else {
        _node_see_func(pass, seer_slice, seer_box, seen_node->a, dims, thread_context);
        _node_see_func(pass, seer_slice, seer_box, seen_node->b, dims, thread_context);
    }
}

void cpu_aabb_tree_see_seen(TayPass *pass, AgentsSlice seer_slice, Box seer_box, int dims, TayThreadContext *thread_context) {
    _node_see_func(pass, seer_slice, seer_box, pass->seen_group->space.cpu_aabb_tree.root, dims, thread_context);
}

static void _see_func(SeeTask *task, TayThreadContext *thread_context) {
    TayPass *pass = task->pass;
    TayGroup *seer_group = pass->seer_group;
    TayGroup *seen_group = pass->seen_group;
    CpuAabbTree *seer_tree = &seer_group->space.cpu_aabb_tree;

    int min_dims = (seer_group->space.dims < seen_group->space.dims) ?
                   seer_group->space.dims :
                   seen_group->space.dims;

    TayRange seers_range = tay_threads_range(pass->seer_group->space.count, task->thread_i);
    unsigned seer_i = seers_range.beg;

    while (seer_i < seers_range.end) {
        TayAgentTag *seer = (TayAgentTag *)(seer_group->storage + seer_group->agent_size * seer_i);
        TreeNode *seer_node = seer_tree->nodes + seer->part_i;

        Box seer_box = seer_node->box;
        for (int i = 0; i < min_dims; ++i) {
            seer_box.min.arr[i] -= pass->radii.arr[i];
            seer_box.max.arr[i] += pass->radii.arr[i];
        }

        unsigned cell_end_seer_i = _minu(seer_node->first_agent_i + seer_node->count, seers_range.end);

        AgentsSlice seer_slice = {
            seer_group->storage,
            seer_group->agent_size,
            seer_i,
            cell_end_seer_i,
        };

        pass->seen_func(pass, seer_slice, seer_box, min_dims, thread_context);

        seer_i = cell_end_seer_i;
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
