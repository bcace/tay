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

static inline int _is_node_leaf(TreeNode *node) {
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

static void _init_leaf_node(TreeNode *node, TayAgentTag *agent) {
    node->agent = agent;
    node->r = 0;
    node->box.min = float4_agent_min(agent);
    node->box.max = float4_agent_max(agent);
}

static void _init_branch_node(TreeNode *node, TreeNode *a, TreeNode *b) {
    node->l = a;
    node->r = b;
    node->box.min = _float4_min(a.min, b.min);
    node->box.max = _float4_max(a.max, b.max);
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

static TreeNode *_find_best_sibling(TreeNode *node, Box box, int dims, TreeNode **parent) {
    if (_is_node_leaf(node))
        return node;
    *parent = node;
    if (_increase_in_volume(box, node->l->box, dims) < _increase_in_volume(box, node->r->box, dims))
        return _find_best_sibling(node->l, box, dims, parent);
    else
        return _find_best_sibling(node->r, box, dims, parent);
}

void cpu_aabb_tree_sort(Space *space, TayGroup *groups) {
    CpuAABBTree *tree = &space->cpu_aabb_tree;

    tree->nodes = space->shared;
    tree->nodes_count = 0;
    tree->max_cells = space->shared_size / (int)sizeof(TreeNode);
    tree->root_node = 0;

    for (int group_i = 0; group_i < TAY_MAX_GROUPS; ++group_i) {
        TayGroup *group = groups + group_i;

        if (group->space != space) /* sort only agents belonging to this space */
            continue;

        TayAgentTag *next = space->first[group_i];
        while (next) {
            TayAgentTag *agent = next;
            next = next->next;

            if (tree->nodes_count == 0) { /* first node */
                TreeNode *node = tree->nodes + tree->nodes_count++;
                _init_leaf_node(node, agent);
                tree->root_node = node;
            }
            else {
                TreeNode *parent = 0;
                TreeNode *sibling = _find_best_sibling(tree->root_node, box, space->dims, &parent);

                TreeNode *new_node = tree->nodes + tree->nodes_count++;
                TreeNode *new_parent = tree->nodes + tree->nodes_count++;

                _init_leaf_node(new_node, agent);
                _init_branch_node(new_parent, sibling, new_node);

                if (parent == 0)
                    tree->root_node = new_parent;
                else {
                    if (sibling == parent->l)
                        parent->l = new_parent;
                    else
                        parent->r = new_parent;
                }
            }
        }

        space->first[group_i] = 0;
        space->counts[group_i] = 0;
    }
}
