#include "tree.h"

#include <math.h>

static int is_num(const node_t* n, double v)
{
    return n && n->node_type == TYPE_NUM && n->value.d_value == v;
}

#define L node->left
#define R node->right

static node_t* optimize_add_node(node_t* node, int* changed)
{
    if (is_num(L, 0.0) && !is_num(R, 0.0))
    {
        node_t* res = R;
        tree_delete_node(L, 0);
        node->left  = NULL;
        node->right = NULL;
        node_dtor(node);
        *changed = 1;
        return res;
    }

    if (is_num(R, 0.0) && !is_num(L, 0.0))
    {
        node_t* res = L;
        tree_delete_node(R, 0);
        node->left  = NULL;
        node->right = NULL;
        node_dtor(node);
        *changed = 1;
        return res;
    }

    if (is_num(L, 0.0) && is_num(R, 0.0))
    {
        tree_delete_node(L, 0);
        tree_delete_node(R, 0);
        L = R = NULL;
        node->node_type     = TYPE_NUM;
        node->value.d_value = 0.0;
        *changed = 1;
        return node;
    }

    return node;
}

static node_t* optimize_mul_node(node_t* node, int* changed)
{
    if (is_num(L, 0.0) || is_num(R, 0.0))
    {
        if (L) tree_delete_node(L, 0);
        if (R) tree_delete_node(R, 0);
        L = R = NULL;
        node->node_type     = TYPE_NUM;
        node->value.d_value = 0.0;
        *changed = 1;
        return node;
    }

    if (is_num(L, 1.0) && !is_num(R, 1.0))
    {
        node_t* res = R;
        tree_delete_node(L, 0);
        node->left  = NULL;
        node->right = NULL;
        node_dtor(node);
        *changed = 1;
        return res;
    }

    if (is_num(R, 1.0) && !is_num(L, 1.0))
    {
        node_t* res = L;
        tree_delete_node(R, 0);
        node->left  = NULL;
        node->right = NULL;
        node_dtor(node);
        *changed = 1;
        return res;
    }

    if (is_num(L, 1.0) && is_num(R, 1.0))
    {
        tree_delete_node(L, 0);
        tree_delete_node(R, 0);
        L = R = NULL;
        node->node_type     = TYPE_NUM;
        node->value.d_value = 1.0;
        *changed = 1;
        return node;
    }

    return node;
}

static node_t* optimize_pow_node(node_t* node, int* changed)
{
    if (is_num(R, 0.0))
    {
        if (L) tree_delete_node(L, 0);
        if (R) tree_delete_node(R, 0);
        L = R = NULL;
        node->node_type     = TYPE_NUM;
        node->value.d_value = 1.0;
        *changed = 1;
        return node;
    }

    if (is_num(R, 1.0))
    {
        node_t* res = L;
        tree_delete_node(R, 0);
        node->left  = NULL;
        node->right = NULL;
        node_dtor(node);
        *changed = 1;
        return res;
    }

    if (is_num(L, 1.0))
    {
        if (L) tree_delete_node(L, 0);
        if (R) tree_delete_node(R, 0);
        L = R = NULL;
        node->node_type     = TYPE_NUM;
        node->value.d_value = 1.0;
        *changed = 1;
        return node;
    }

    return node;
}

static node_t* optimize_subtree(node_t* node, int* changed)
{
    if (!node) return NULL;

    if (L)
        L = optimize_subtree(L, changed);
    if (R)
        R = optimize_subtree(R, changed);

    if (node->node_type != TYPE_OP)
        return node;

    switch (node->value.op)
    {
        case OP_ADD:
            return optimize_add_node(node, changed);
        case OP_MUL:
            return optimize_mul_node(node, changed);
        case OP_POW:
            return optimize_pow_node(node, changed);
        default:
            return node;
    }
}

#undef L
#undef R

void tree_optimize(tree_t* tree)
{
    if (!tree)
        return;

    int changed = 0;
    tree->root = optimize_subtree(tree->root, &changed);
}

