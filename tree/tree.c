#include "tree.h"

#include <ctype.h>
#include <math.h>

#define DECLARE_OP_DESC(name, text, id) { text, 0, name },

static op_t g_op_desc[] = {
    OPERATION_LIST(DECLARE_OP_DESC)
};

static int g_op_hashes_inited = 0;

static void init_op_hashes(void)
{
    if (g_op_hashes_inited)
        return;
    size_t n = sizeof(g_op_desc) / sizeof(g_op_desc[0]);
    for (size_t i = 0; i < n; ++i)
        g_op_desc[i].hash = sdbm(g_op_desc[i].str);
    g_op_hashes_inited = 1;
}

static int op_from_token_hash(const char* tok, node_operations_e* out_op)
{
    init_op_hashes();
    size_t h = sdbm(tok);
    size_t n = sizeof(g_op_desc) / sizeof(g_op_desc[0]);
    for (size_t i = 0; i < n; ++i)
    {
        if (g_op_desc[i].hash == h)
        {
            if (out_op)
                *out_op = g_op_desc[i].op;
            return 1;
        }
    }
    return 0;
}

static void skip_ws(const char **buf)
{
    if (!buf || !*buf) return;

    const char *p = *buf;
    while (*p != '\0' && isspace((unsigned char)*p))
        ++p;

    *buf = p;
}

size_t sdbm(const char * str) 
{
    size_t hash = 0;
    int c = 0;

    while ((c = *str++) != '\0') 
    {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

bool is_same(double a, double b) 
{
    return fabs(a - b) < FLT_ERR;
}

static err_t vars_ensure_capacity(tree_t *tree, size_t min_capacity)
{
    if (!tree)
        return ERR_BAD_ARG;

    size_t new_cap = (tree->var_capacity > 0) ? (tree->var_capacity * 2) : VARS_MIN_CAP;
    if (new_cap < min_capacity)
        new_cap = min_capacity;

    var_t *new_arr = (var_t*)realloc(tree->variables, new_cap * sizeof(var_t));
    if (!new_arr)
        return ERR_ALLOC;

    if (new_cap > tree->var_capacity)
    {
        size_t old_cap = tree->var_capacity;
        memset(new_arr + old_cap, 0, (new_cap - old_cap) * sizeof(var_t));
    }

    tree->variables    = new_arr;
    tree->var_capacity = new_cap;

    return OK;
}

static void vars_free(tree_t *tree)
{
    if (!tree)
        return;

    if (tree->variables)
    {
        for (size_t i = 0; i < tree->var_amount; ++i)
        {
            if (tree->variables[i].name)
            {
                free(tree->variables[i].name);
                tree->variables[i].name = NULL;
            }
        }

        free(tree->variables);
        tree->variables = NULL;
    }

    tree->var_amount   = 0;
    tree->var_capacity = 0;
}

err_t node_ctor(node_t** node)
{
    if (!CHECK(ERROR, node != NULL, "node_ctor: node is NULL"))
        return ERR_BAD_ARG;

    *node = (node_t*)calloc(1, sizeof(**node));
    if (*node == NULL)
        return ERR_ALLOC;

    (*node)->node_type     = TYPE_NUM;
    (*node)->value.d_value = 0.0;
    (*node)->left          = NULL;
    (*node)->right         = NULL;
    (*node)->rank          = 0;

    return OK;
}

err_t node_dtor(node_t* node)
{
    if (!CHECK(ERROR, node != NULL, "node_dtor: node is NULL"))
        return ERR_BAD_ARG;

    if (node->node_type == TYPE_VAR && node->value.var.name != NULL)
    {
        free(node->value.var.name);
        node->value.var.name = NULL;
    }

    free(node);

    return OK;
}

err_t tree_ctor(tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_ctor: tree is NULL"))
        return ERR_BAD_ARG;

    tree->nodes_amount = 0;
    tree->root         = NULL;
    tree->variables    = NULL;
    tree->var_amount   = 0;
    tree->var_capacity = 0;
    return OK;
}

err_t tree_dtor(tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_dtor: tree is NULL"))
        return ERR_BAD_ARG;

    return tree_clear(tree);
}

err_t tree_verify(const tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_verify: tree is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR, tree->nodes_amount < MAX_RECURSION_LIMIT,
               "tree_verify: tree->nodes_amount too large"))
        return ERR_CORRUPT;

    return OK;
}

static const char* op_to_str(node_operations_e op)
{
    switch (op)
    {
        case OP_ADD:  return "+";
        case OP_SUB:  return "-";
        case OP_MUL:  return "*";
        case OP_DIV:  return "/";
        case OP_POW:  return "^";

        case OP_SIN:  return "sin";
        case OP_COS:  return "cos";
        case OP_TAN:  return "tan";
        case OP_COT:  return "cot";

        case OP_SINH: return "sinh";
        case OP_COSH: return "cosh";
        case OP_TANH: return "tanh";
        case OP_COTH: return "coth";

        case OP_LOG:  return "log";
        case OP_LN:   return "ln";
        case OP_SQRT: return "sqrt";

        case OP_ASIN: return "asin";
        case OP_ACOS: return "acos";
        case OP_ATAN: return "atan";
        case OP_ACOT: return "acot";

        case OP_NOP:
        default:      return "?";
    }
}

static err_t tree_fprint_node(FILE* out, const node_t * const node, size_t iter)
{
    if (!CHECK(ERROR, out  != NULL, "tree_fprint_node: out is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR, iter <= MAX_RECURSION_LIMIT,
               "tree_fprint_node: recursion limit exceeded on node %p", node))
        return ERR_CORRUPT;

    if (node == NULL)
    {
        fprintf(out, "nil");
        if (iter == 0)
            fputc('\n', out);
        return OK;
    }

    fputc('(', out);

    switch (node->node_type)
    {
        case TYPE_NUM:
            fprintf(out, "\"%.15g\"", node->value.d_value);
            break;

        case TYPE_VAR:
            fprintf(out, "\"%s\"", node->value.var.name ? node->value.var.name : "");
            break;

        case TYPE_OP:
        default:
            fprintf(out, "\"%s\"", op_to_str(node->value.op));
            break;
    }

    if (node->left)
    {
        if (tree_fprint_node(out, node->left, iter + 1) != OK)
            return ERR_CORRUPT;
    }
    else
    {
        fprintf(out, "nil");
    }

    if (node->right)
    {
        if (tree_fprint_node(out, node->right, iter + 1) != OK)
            return ERR_CORRUPT;
    }
    else
    {
        fprintf(out, "nil");
    }

    fputc(')', out);

    if (iter == 0)
        fputc('\n', out);

    return OK;
}

err_t tree_fprint(const char* filename, const tree_t * const tree)
{
    if (!CHECK(ERROR, filename  != NULL, "tree_fprint: out is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR, tree != NULL, "tree_fprint: tree is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR, tree_verify(tree) == OK,
               "tree_fprint: tree verification failed"))
        return ERR_CORRUPT;

    FILE* file = load_file(filename, "wb");
    if (!CHECK(ERROR, file != NULL, "tree_write_file: load_file failed"))
        return ERR_BAD_ARG;

    if (tree->root != NULL)
    {
        if (!CHECK(ERROR, tree_fprint_node(file, tree->root, 0) == OK,
                   "tree_fprint: failed to print tree nodes"))
        {
            fclose(file);
            return ERR_CORRUPT;
        }
    }
    else
    {
        fprintf(file, "nil\n");
    }

    fclose(file);

    return OK;
}
err_t tree_delete_node(node_t * node, size_t iter)
{
    if (!CHECK(ERROR, node != NULL, "tree_delete_node: node is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR, iter <= MAX_RECURSION_LIMIT,
               "tree_delete_node: recursion limit exceeded on node %p", node))
        return ERR_CORRUPT;

    if (node->left != NULL)
        if (!CHECK(ERROR, tree_delete_node(node->left, iter + 1) == OK,
                   "tree_delete_node: failed to delete left node"))
            return ERR_CORRUPT;

    if (node->right != NULL)
        if (!CHECK(ERROR, tree_delete_node(node->right, iter + 1) == OK,
                   "tree_delete_node: failed to delete right node"))
            return ERR_CORRUPT;

    node_dtor(node);

    return OK;
}

err_t tree_clear(tree_t * const tree)
{
    if (!CHECK(ERROR, tree != NULL, "tree_clear: tree is NULL"))
        return ERR_BAD_ARG;

    if (tree->root != NULL)
    {
        if (!CHECK(ERROR, tree_delete_node(tree->root, 0) == OK,
                   "tree_clear: failed to delete nodes"))
            return ERR_CORRUPT;
    }

    tree->root         = NULL;
    tree->nodes_amount = 0;

    vars_free(tree);

    return OK;
}

err_t tree_insert(tree_t * const tree, node_t * const node)
{
    if (!CHECK(ERROR, tree != NULL, "tree_insert: tree is NULL"))
        return ERR_BAD_ARG;

    if (!CHECK(ERROR, node != NULL, "tree_insert: node is NULL"))
        return ERR_BAD_ARG;

    tree->nodes_amount++;
    if (tree->root == NULL)
        tree->root = node;

    return OK;
}

var_t* get_or_create_var(tree_t* tree, const char* name)
{
    size_t h = sdbm(name);
    for (size_t i = 0; i < tree->var_amount; ++i)
    {
        if (tree->variables[i].hash == h)
            return &tree->variables[i];
    }

    if (vars_ensure_capacity(tree, tree->var_amount + 1) != OK)
        return NULL;

    var_t* v = &tree->variables[tree->var_amount++];
    v->name  = strdup(name);
    v->hash  = h;
    v->value = 0.0;

    return v;
}

size_t get_op_rank(node_operations_e op)
{
    switch (op)
    {
        case OP_ADD:
        case OP_SUB:  return 10;

        case OP_MUL:
        case OP_DIV:  return 20;

        case OP_POW:  return 30;

        case OP_SIN:
        case OP_COS:
        case OP_TAN:
        case OP_COT:
        case OP_SINH:
        case OP_COSH:
        case OP_TANH:
        case OP_COTH:
        case OP_LOG:
        case OP_LN:
        case OP_SQRT:
        case OP_ASIN:
        case OP_ACOS:
        case OP_ATAN:
        case OP_ACOT: return 40;

        case OP_NOP:
        default:      return 0;
    }
}

static int init_node_from_token(tree_t* tree, node_t* node, const char* tok)
{
    int is_number = 1;
    size_t len = strlen(tok);
    for (size_t i = 0; i < len; ++i)
    {
        if (!(isdigit((unsigned char)tok[i]) || tok[i] == '.'))
        {
            is_number = 0;
            break;
        }
    }

    if (is_number)
    {
        node->node_type     = TYPE_NUM;
        node->value.d_value = strtod(tok, NULL);
        node->rank          = 100;
        return 1;
    }

    if (isalpha((unsigned char)tok[0]) && len == 1)
    {
        node->node_type = TYPE_VAR;
        var_t* v = get_or_create_var(tree, tok);
        if (!v)
            return 0;
        node->value.var.name  = strdup(v->name);
        node->value.var.hash  = v->hash;
        node->value.var.value = 0.0;
        node->rank            = 100;
        return 1;
    }

    node_operations_e op = OP_NOP;
    if (op_from_token_hash(tok, &op))
    {
        node->node_type = TYPE_OP;
        node->value.op  = op;
        node->rank      = get_op_rank(op);
        return 1;
    }
    return 0;
}

static int read_quoted_token(const char** buf, char* out, size_t max_len)
{
    skip_ws(buf);

    const char* p = *buf;
    if (*p != '"')
        return 0;
    
    ++p;
    size_t n = 0;
    while (*p != '\0' && *p != '"' && n + 1 < max_len)
    {
        out[n++] = *p;
        ++p;
    }

    if (*p != '"')
        return 0;
    
    out[n] = '\0';
    ++p;
    *buf = p;

    return 1;
}

static int match_str(const char** buf, const char* s)
{
    skip_ws(buf);
    size_t len = strlen(s);
    if (strncmp(*buf, s, len) == 0)
    {
        *buf += len;
        return 1;
    }
    return 0;
}

static int match_nil(const char** buf)
{ 
    skip_ws(buf);
    const char* p = *buf;
    if (p[0] == 'n' && p[1] == 'i' && p[2] == 'l')
        { *buf += 3; return 1; }

    return 0;
}

static node_t* parse_node(tree_t* tree, const char** buf, err_t* err)
{
    if (!err)
        return NULL; 

    if (match_nil(buf))
        return NULL;

    skip_ws(buf);
    if (**buf != '(') 
        { *err = ERR_CORRUPT; return NULL; }

    (*buf)++;
    char token[128] = { 0 };
    if (!read_quoted_token(buf, token, sizeof(token))) 
        { *err = ERR_CORRUPT; return NULL; }

    node_t* node = NULL;
    *err = node_ctor(&node);
    if (!node || *err != OK)
        { *err = ERR_CORRUPT; return NULL; }

    if (!init_node_from_token(tree, node, token)) 
        { *err = ERR_CORRUPT; return NULL; }

    tree->nodes_amount++;
    node->left  = parse_node(tree, buf, err);
    if (*err != OK) return NULL;

    node->right = parse_node(tree, buf, err);
    if (*err != OK) return NULL; 
    
    skip_ws(buf); 

    if (**buf != ')')
        { *err = ERR_CORRUPT; return node; }

    (*buf)++;
    return node;
}

err_t tree_read_file(tree_t* tree, const char* filename, logging_level level)
{
    if (!CHECK(ERROR, tree != NULL, "tree_read_file: tree == NULL")) 
        return ERR_BAD_ARG;
    unused level;

    size_t fsize = (size_t)get_file_size_stat(filename) + 1;
    operational_data_t op_data = {  };
    memset(&op_data, 0, sizeof(op_data));
    op_data.buffer_size = fsize;
    op_data.buffer      = (char*)calloc(1, fsize);

    FILE* file  = load_file(filename, "rb");
    size_t read = read_file(file, &op_data);
    if (read == 0)
    {
        free(op_data.buffer);
        fclose(file);
        return ERR_CORRUPT;
    }
 
    err_t parse_err = OK;
    size_t curr_pos = 0; 
    if (op_data.buffer_size >= 3 &&
        (unsigned char)op_data.buffer[0] == 0xEF &&
        (unsigned char)op_data.buffer[1] == 0xBB &&
        (unsigned char)op_data.buffer[2] == 0xBF) {
        curr_pos = 3;
    }
    const char* buf_ptr = op_data.buffer + curr_pos;
    tree_clear(tree);
    tree->root = parse_node(tree, &buf_ptr, &parse_err);

    if (tree->root == NULL || parse_err != OK)
    {
        size_t err_pos = op_data.error_msg[0] ? op_data.error_pos : (size_t)(buf_ptr - op_data.buffer);
        size_t line = 1;
        size_t col  = 1;

        for (size_t i = 0; i < err_pos && i < op_data.buffer_size; ++i)
        {
            if (op_data.buffer[i] == '\n') { line++; col = 1; }
            else                           { col++; }
        }

        const char* msg = op_data.error_msg[0] ? op_data.error_msg
                                               : "unknown parse error";

        log_printf(ERROR,
                   "Parse error in \"%s\" at %zu:%zu (offset %zu): %s",
                   filename ? filename : "<input>",
                   line, col, err_pos, msg);

        printf("Parse error in \"%s\" at %zu:%zu (offset %zu): %s",
               filename ? filename : "<input>",
               line, col, err_pos, msg);
 
        free(op_data.buffer);
        fclose(file);
        return ERR_CORRUPT;
    }

    free(op_data.buffer);
    fclose(file);
    return OK;
}

node_t* clone_subtree(const node_t* src,
                      tree_t*       dst,
                      size_t        depth,
                      err_t*        err)
{
    if (!src || !dst || !err)
        return NULL;

    if (*err != OK)
        return NULL;

    if (depth > MAX_RECURSION_LIMIT)
    {
        *err = ERR_CORRUPT;
        return NULL;
    }

    node_t *node = NULL;
    err_t rc = node_ctor(&node);
    if (rc != OK)
    {
        *err = rc;
        return NULL;
    }

    node->node_type = src->node_type;
    node->left      = NULL;
    node->right     = NULL;

    if (src->node_type == TYPE_VAR && src->value.var.name != NULL)
    {
        size_t len = strlen(src->value.var.name);
        node->value.var.name = (char*)calloc(len + 1, sizeof(char));
        if (!node->value.var.name)
        {
            free(node);
            *err = ERR_ALLOC;
            return NULL;
        }
        memcpy(node->value.var.name, src->value.var.name, len);
        node->value.var.name[len] = '\0';
        node->value.var.hash  = src->value.var.hash;
        node->value.var.value = src->value.var.value;
    }
    else
    {
        node->value = src->value;
    }

    dst->nodes_amount++;

    if (src->left)
    {
        node->left = clone_subtree(src->left, dst, depth + 1, err);
        if (*err != OK)
        {
            tree_delete_node(node, 0);
            return NULL;
        }
    }

    if (src->right)
    {
        node->right = clone_subtree(src->right, dst, depth + 1, err);
        if (*err != OK)
        {
            tree_delete_node(node, 0);
            return NULL;
        }
    }

    return node;
}

