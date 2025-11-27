#include "differentiation.h"

static node_t* new_num(tree_t* out_tree, double v, err_t* err)
{
    if (*err != OK)
        return NULL;

    node_t* n = NULL;
    *err = node_ctor(&n);
    if (*err != OK || !n)
    {
        *err = ERR_ALLOC;
        return NULL;
    }

    n->node_type     = TYPE_NUM;
    n->value.d_value = v;
    n->left          = NULL;
    n->right         = NULL;
    n->rank          = 100;

    out_tree->nodes_amount++;
    return n;
}

static node_t* new_op(tree_t* out_tree, node_operations_e op,
                      node_t* left, node_t* right, err_t* err)
{
    if (*err != OK)
        return NULL;

    node_t* n = NULL;
    *err = node_ctor(&n);
    if (*err != OK || !n)
    {
        *err = ERR_ALLOC;
        return NULL;
    }

    n->node_type = TYPE_OP;
    n->value.op  = op;
    n->left      = left;
    n->right     = right;
    n->rank      = get_op_rank(op);

    out_tree->nodes_amount++;
    return n;
}

static int subtree_holds_var(const node_t* node, size_t var_hash)
{
    if (!node)
        return 0;

    if (node->node_type == TYPE_VAR &&
        node->value.var.hash == var_hash)
        return 1;

    return subtree_holds_var(node->left,  var_hash) ||
           subtree_holds_var(node->right, var_hash);
}

static node_t* diff_node(const node_t* node,
                         tree_t*       out_tree,
                         size_t        var_hash,
                         err_t*        err);

#define cL clone_subtree(node->left,  out_tree, 0, err)
#define cR clone_subtree(node->right, out_tree, 0, err)
#define dL diff_node(node->left,  out_tree, var_hash, err)
#define dR diff_node(node->right, out_tree, var_hash, err)

#define CONST_(num_)      new_num(out_tree, (num_), err)

#define ADD_(L_, R_)      new_op(out_tree, OP_ADD,  (L_), (R_), err)
#define SUB_(L_, R_)      new_op(out_tree, OP_SUB,  (L_), (R_), err)
#define MUL_(L_, R_)      new_op(out_tree, OP_MUL,  (L_), (R_), err)
#define DIV_(L_, R_)      new_op(out_tree, OP_DIV,  (L_), (R_), err)
#define POW_(L_, R_)      new_op(out_tree, OP_POW,  (L_), (R_), err)

#define SIN_(A_)          new_op(out_tree, OP_SIN,  (A_), NULL, err)
#define COS_(A_)          new_op(out_tree, OP_COS,  (A_), NULL, err)
#define TAN_(A_)          new_op(out_tree, OP_TAN,  (A_), NULL, err)
#define COT_(A_)          new_op(out_tree, OP_COT,  (A_), NULL, err)

#define SINH_(A_)         new_op(out_tree, OP_SINH, (A_), NULL, err)
#define COSH_(A_)         new_op(out_tree, OP_COSH, (A_), NULL, err)
#define TANH_(A_)         new_op(out_tree, OP_TANH, (A_), NULL, err)
#define COTH_(A_)         new_op(out_tree, OP_COTH, (A_), NULL, err)

#define LN_(A_)           new_op(out_tree, OP_LN,   (A_), NULL, err)
#define SQRT_(A_)         new_op(out_tree, OP_SQRT, (A_), NULL, err)

static node_t* diff_op(const node_t* node,
                       tree_t*       out_tree,
                       size_t        var_hash,
                       err_t*        err)
{
    switch (node->value.op)
    {
        case OP_ADD:  return ADD_(dL, dR);
        case OP_SUB:  return SUB_(dL, dR);
        case OP_MUL:  return ADD_(MUL_(dL, cR), MUL_(cL, dR));
        case OP_DIV:  return DIV_(SUB_(MUL_(dL, cR), MUL_(cL, dR)), POW_(cR, CONST_(2.0)));

        case OP_POW:
        {
            int left_dep  = subtree_holds_var(node->left,  var_hash);
            int right_dep = subtree_holds_var(node->right, var_hash);

            if (!left_dep && !right_dep)
                return CONST_(0.0);
            else if (left_dep && !right_dep)
                return MUL_(MUL_(cR, POW_(cL, SUB_(cR, CONST_(1.0)))), dL);
            else if (!left_dep && right_dep)
                return MUL_(MUL_(POW_(cL, cR), LN_(cL)), dR);
            else
            {
                node_t* term1 = MUL_(dR, LN_(cL));
                node_t* term2 = MUL_(cR, DIV_(dL, cL));
                node_t* sum   = ADD_(term1, term2);
                return MUL_(POW_(cL, cR), sum);
            }
        }

        case OP_LN:   return DIV_(dL, cL);

        case OP_LOG:
        {
            node_t* term1 = MUL_(DIV_(dR, cR), LN_(cL));
            node_t* term2 = MUL_(DIV_(dL, cL), LN_(cR));
            node_t* num   = SUB_(term1, term2);
            node_t* den   = POW_(LN_(cL), CONST_(2.0));
            return DIV_(num, den);
        }

        case OP_SQRT: return DIV_(dL, MUL_(CONST_(2.0), SQRT_(cL)));

        case OP_SIN:  return MUL_(COS_(cL), dL);
        case OP_COS:  return MUL_(CONST_(-1.0), MUL_(SIN_(cL), dL));
        case OP_TAN:  return DIV_(dL, POW_(COS_(cL), CONST_(2.0)));
        case OP_COT:  return MUL_(CONST_(-1.0), DIV_(dL, POW_(SIN_(cL), CONST_(2.0))));

        case OP_SINH: return MUL_(COSH_(cL), dL);
        case OP_COSH: return MUL_(SINH_(cL), dL);
        case OP_TANH: return DIV_(dL, POW_(COSH_(cL), CONST_(2.0)));
        case OP_COTH: return MUL_(CONST_(-1.0), DIV_(dL, POW_(SINH_(cL), CONST_(2.0))));

        case OP_ASIN: return DIV_(dL, SQRT_(SUB_(CONST_(1.0), POW_(cL, CONST_(2.0)))));
        case OP_ACOS: return MUL_(CONST_(-1.0),
                                  DIV_(dL, SQRT_(SUB_(CONST_(1.0), POW_(cL, CONST_(2.0))))));
        case OP_ATAN: return DIV_(dL, ADD_(CONST_(1.0), POW_(cL, CONST_(2.0))));
        case OP_ACOT: return MUL_(CONST_(-1.0),
                                  DIV_(dL, ADD_(CONST_(1.0), POW_(cL, CONST_(2.0)))));

        case OP_NOP:
        default:
            return node->left ? dL : CONST_(0.0);
    }
}

static node_t* diff_node(const node_t* node,
                         tree_t*       out_tree,
                         size_t        var_hash,
                         err_t*        err)
{
    if (!node || *err != OK)
        return NULL;

    switch (node->node_type)
    {
        case TYPE_NUM:
            return CONST_(0.0);

        case TYPE_VAR:
            if (node->value.var.hash == var_hash)
                return CONST_(1.0);
            else
                return CONST_(0.0);

        case TYPE_OP:
            return diff_op(node, out_tree, var_hash, err);

        default:
            *err = ERR_CORRUPT;
            return NULL;
    }
}

static size_t count_nodes_rec(const node_t* n)
{
    if (!n) return 0;
    return 1 + count_nodes_rec(n->left) + count_nodes_rec(n->right);
}

static err_t tree_derivative_once(const tree_t* in_tree,
                                  tree_t*       out_tree,
                                  size_t        var_hash)
{
    if (!in_tree || !out_tree)
        return ERR_BAD_ARG;

    err_t err = OK;
    out_tree->root         = NULL;
    out_tree->nodes_amount = 0;

    if (!in_tree->root)
        return OK;

    node_t* new_root = diff_node(in_tree->root, out_tree, var_hash, &err);
    if (err != OK || !new_root)
    {
        tree_clear(out_tree);
        return err ? err : ERR_CORRUPT;
    }

    out_tree->root         = new_root;
    out_tree->nodes_amount = count_nodes_rec(new_root);

    return OK;
}

err_t tree_derivative(tree_t* in_tree,
                      tree_t* out_tree,
                      const char * const variable,
                      size_t n)
{
    if (!in_tree || !out_tree || !variable)
        return ERR_BAD_ARG;

    size_t var_hash = sdbm(variable);

    if (n == 0)
    {
        err_t err = OK;
        tree_clear(out_tree);
        out_tree->root         = clone_subtree(in_tree->root, out_tree, 0, &err);
        if (err != OK)
        {
            tree_clear(out_tree);
            return err;
        }
        out_tree->nodes_amount = count_nodes_rec(out_tree->root);
        return OK;
    }

    tree_t current;
    err_t err = tree_ctor(&current);
    if (err != OK)
        return err;

    err = OK;
    current.root         = clone_subtree(in_tree->root, &current, 0, &err);
    if (err != OK)
    {
        tree_clear(&current);
        return err;
    }
    current.nodes_amount = count_nodes_rec(current.root);

    for (size_t i = 0; i < n; ++i)
    {
        tree_clear(out_tree);
        err = tree_derivative_once(&current, out_tree, var_hash);
        if (err != OK)
        {
            tree_clear(&current);
            tree_clear(out_tree);
            return err;
        }

        if (i + 1 < n)
        {
            tree_clear(&current);
            err = OK;
            current.root         = clone_subtree(out_tree->root, &current, 0, &err);
            if (err != OK)
            {
                tree_clear(&current);
                tree_clear(out_tree);
                return err;
            }
            current.nodes_amount = count_nodes_rec(current.root);
        }
    }

    tree_clear(&current);
    return OK;
}

#undef cL
#undef cR
#undef dL
#undef dR
#undef CONST_
#undef ADD_
#undef SUB_
#undef MUL_
#undef DIV_
#undef POW_
#undef SIN_
#undef COS_
#undef TAN_
#undef COT_
#undef SINH_
#undef COSH_
#undef TANH_
#undef COTH_
#undef LN_
#undef SQRT_
