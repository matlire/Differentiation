#include "dump.h"

static size_t s_img_counter = 0;

typedef struct NodeInfo
{
    const node_t *node;
    size_t        id;
    size_t        xpos;
} NodeInfo;

static void node_dump_label(const node_t *node,
                            char *type_buf,  size_t type_buf_size,
                            char *value_buf, size_t value_buf_size)
{
    if (!node || !type_buf || type_buf_size == 0 ||
        !value_buf || value_buf_size == 0)
        return;

    switch (node->node_type)
    {
        case TYPE_OP:
        {
            const char *op_str = "?";
            switch (node->value.op)
            {
                case OP_ADD:  op_str = "+";    break;
                case OP_SUB:  op_str = "-";    break;
                case OP_MUL:  op_str = "*";    break;
                case OP_DIV:  op_str = "/";    break;
                case OP_POW:  op_str = "^";    break;
                case OP_SIN:  op_str = "sin";  break;
                case OP_COS:  op_str = "cos";  break;
                case OP_TAN:  op_str = "tan";  break;
                case OP_COT:  op_str = "cot";  break;
                case OP_SINH: op_str = "sinh"; break;
                case OP_COSH: op_str = "cosh"; break;
                case OP_TANH: op_str = "tanh"; break;
                case OP_COTH: op_str = "coth"; break;
                case OP_LOG:  op_str = "log";  break;
                case OP_LN:   op_str = "ln";   break;
                case OP_SQRT: op_str = "sqrt"; break;
                case OP_ASIN: op_str = "asin"; break;
                case OP_ACOS: op_str = "acos"; break;
                case OP_ATAN: op_str = "atan"; break;
                case OP_ACOT: op_str = "acot"; break;
                case OP_NOP:
                default:      op_str = "?";    break;
            }
            snprintf(type_buf,  type_buf_size,  "OP");
            snprintf(value_buf, value_buf_size,
                     "%s (id=%d)", op_str, (int)node->value.op);
            break;
        }

        case TYPE_VAR:
        {
            snprintf(type_buf, type_buf_size, "VAR");
            if (node->value.var.name && node->value.var.name[0] != '\0')
            {
                snprintf(value_buf, value_buf_size,
                         "name=\"%s\" hash=%.0f val=%.15g",
                         node->value.var.name,
                         (double)node->value.var.hash,
                         node->value.var.value);
            }
            else
            {
                snprintf(value_buf, value_buf_size,
                         "name=\"?\" hash=%.0f val=%.15g",
                         (double)node->value.var.hash,
                         node->value.var.value);
            }
            break;
        }

        case TYPE_NUM:
        default:
        {
            snprintf(type_buf,  type_buf_size,  "NUM");
            snprintf(value_buf, value_buf_size, "%.15g", node->value.d_value);
            break;
        }
    }
}

void tree_dump_reset(const char *filename)
{
    if (!filename) return;
    FILE *f = fopen(filename, "w");
    if (f) fclose(f);
    s_img_counter = 0;
}

static size_t find_index_by_ptr(NodeInfo *arr, size_t n, const node_t *p)
{
    for (size_t i = 0; i < n; ++i)
        if (arr[i].node == p) return i;
    return (size_t)-1;
}

static void assign_inorder_xpos(const node_t *node, NodeInfo *arr, size_t n, size_t *counter)
{
    if (!node) return;
    assign_inorder_xpos(node->left,  arr, n, counter);
    size_t idx = find_index_by_ptr(arr, n, node);
    if (idx != (size_t)-1) arr[idx].xpos = (*counter)++;
    assign_inorder_xpos(node->right, arr, n, counter);
}

void tree_dump_graphviz(const tree_t *tree, const char *title, const char *filename)
{
    if (!tree || !filename) return;

    const node_t *root = tree->root;
    char dot_path[512], svg_name[64], svg_path[512];
    snprintf(svg_name, sizeof(svg_name), "img%zu.svg", s_img_counter++);
    snprintf(dot_path, sizeof(dot_path), "temp/graph.dot");
    snprintf(svg_path, sizeof(svg_path), "temp/t%s", svg_name);

    FILE *dot = fopen(dot_path, "w");
    if (!dot) return;

    const char *EDGE_LEFT  = "#98A2B3";
    const char *EDGE_RIGHT = "#98A2B3";
    const char *OUT_ROOT   = "#16A34A";
    const char *OUT_NODE   = "#475467";
    const char *FILL_NODE  = "#F9FAFB";
    const char *FILL_ROOT  = "#E6F4EA";
    const char *CELL_BG    = "#FFFFFF";
    const char *TABLE_BRD  = "#D0D5DD";
    const char *TXT_COLOR  = "#111827";

    fprintf(dot, "digraph G {\n");
    fprintf(dot, "rankdir=TB;\n");
    fprintf(dot, "bgcolor=\"white\";\n");
    fprintf(dot, "labelloc=t;\n");
    fprintf(dot, "labeljust=l;\n");
    fprintf(dot, "fontname=\"monospace\";\n");
    fprintf(dot, "fontsize=18;\n");

    fprintf(dot,
            "node  [shape=box, style=\"rounded,filled\", color=\"%s\", fillcolor=\"%s\", "
            "fontname=\"monospace\", fontsize=10];\n",
            OUT_NODE, FILL_NODE);
    fprintf(dot,
            "edge  [color=\"#98A2B3\", penwidth=1.7, arrowsize=0.8, arrowhead=vee, "
            "fontname=\"monospace\", fontsize=9];\n");

    if (!root)
    {
        fprintf(dot,
                "empty [label=\"<empty tree>\", color=\"#9CA3AF\", "
                "fontcolor=\"#9CA3AF\", fillcolor=\"#F3F4F6\"];\n");
        fprintf(dot, "}\n");
        fclose(dot);

        char cmd_empty[4096];
        snprintf(cmd_empty, sizeof(cmd_empty), "dot -T svg \"%s\" -o \"%s\"",
                 dot_path, svg_path);
        system(cmd_empty);

        FILE *html = fopen(filename, "a");
        if (html)
        {
            fprintf(html, "<h2>%s</h2>\n", title ? title : "Tree");
            fprintf(html, "<h3>Nodes: 0</h3>\n");
            fprintf(html, "<h3>Root: (null)</h3>\n");
            fprintf(html, "<img src=\"temp/%s\" />\n", svg_name);
            fclose(html);
        }
        return;
    }

    size_t cap_nodes  = 64;
    size_t cap_queue  = 64;
    size_t n          = 0;
    size_t head       = 0;
    size_t tail       = 0;

    NodeInfo *nodes         = (NodeInfo*)calloc(cap_nodes, sizeof(NodeInfo));
    const node_t **q_nodes  = (const node_t**)calloc(cap_queue, sizeof(const node_t*));
    if (!nodes || !q_nodes)
    {
        free(nodes);
        free(q_nodes);
        fclose(dot);
        return;
    }

    q_nodes[tail++] = root;

    while (head < tail)
    {
        const node_t *cur = q_nodes[head++];

        if (n == cap_nodes)
        {
            size_t new_cap = cap_nodes * 2;
            NodeInfo *new_nodes = (NodeInfo*)realloc(nodes, new_cap * sizeof(NodeInfo));
            if (!new_nodes)
            {
                free(nodes);
                free(q_nodes);
                fclose(dot);
                return;
            }
            nodes = new_nodes;
            cap_nodes = new_cap;
        }

        nodes[n].node = cur;
        nodes[n].id   = n;
        nodes[n].xpos = 0;
        n++;

        if (cur->left)
        {
            if (tail == cap_queue)
            {
                size_t new_qcap = cap_queue * 2;
                const node_t **new_q =
                    (const node_t**)realloc(q_nodes, new_qcap * sizeof(const node_t*));
                if (!new_q)
                {
                    free(nodes);
                    free(q_nodes);
                    fclose(dot);
                    return;
                }
                q_nodes   = new_q;
                cap_queue = new_qcap;
            }
            q_nodes[tail++] = cur->left;
        }

        if (cur->right)
        {
            if (tail == cap_queue)
            {
                size_t new_qcap = cap_queue * 2;
                const node_t **new_q =
                    (const node_t**)realloc(q_nodes, new_qcap * sizeof(const node_t*));
                if (!new_q)
                {
                    free(nodes);
                    free(q_nodes);
                    fclose(dot);
                    return;
                }
                q_nodes   = new_q;
                cap_queue = new_qcap;
            }
            q_nodes[tail++] = cur->right;
        }
    }

    free(q_nodes);

    size_t counter = 0;
    assign_inorder_xpos(root, nodes, n, &counter);

    for (size_t i = 0; i < n; ++i)
    {
        const node_t *p = nodes[i].node;
        const char *outline = (p == root) ? OUT_ROOT : OUT_NODE;
        const char *fill    = (p == root) ? FILL_ROOT : FILL_NODE;

        char type_str[32]  = {0};
        char value_str[128] = {0};
        node_dump_label(p, type_str, sizeof(type_str),
                           value_str, sizeof(value_str));

        fprintf(dot,
            "n%zu [shape=plain, color=\"%s\", fillcolor=\"%s\", penwidth=2.0, label=<"
            "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\" COLOR=\"%s\">"
            "<TR><TD COLSPAN=\"2\" BGCOLOR=\"%s\"><B><FONT COLOR=\"%s\">node</FONT></B></TD></TR>"
            "<TR><TD ALIGN=\"LEFT\">addr</TD><TD ALIGN=\"LEFT\">%p</TD></TR>"
            "<TR><TD ALIGN=\"LEFT\">node_type</TD><TD ALIGN=\"LEFT\">%s (%d)</TD></TR>"
            "<TR><TD ALIGN=\"LEFT\">rank</TD><TD ALIGN=\"LEFT\">%zu</TD></TR>"
            "<TR><TD ALIGN=\"LEFT\">value</TD><TD ALIGN=\"LEFT\">%s</TD></TR>"
            "<TR><TD PORT=\"L\" ALIGN=\"LEFT\">L: %p</TD>"
            "<TD PORT=\"R\" ALIGN=\"LEFT\">R: %p</TD></TR>"
            "</TABLE>"
            ">];\n",
            nodes[i].id,
            outline, fill,
            TABLE_BRD, CELL_BG, TXT_COLOR,
            (void*)p,
            type_str, (int)p->node_type,
            p->rank,
            value_str,
            (void*)p->left, (void*)p->right);
    }

    for (size_t i = 0; i < n; ++i)
    {
        const node_t *p = nodes[i].node;
        if (p->left)
        {
            size_t j = find_index_by_ptr(nodes, n, p->left);
            if (j != (size_t)-1)
                fprintf(dot,
                        "n%zu -> n%zu [color=\"%s\", penwidth=1.9];\n",
                        nodes[i].id, nodes[j].id, EDGE_LEFT);
        }
        if (p->right)
        {
            size_t j = find_index_by_ptr(nodes, n, p->right);
            if (j != (size_t)-1)
                fprintf(dot,
                        "n%zu -> n%zu [color=\"%s\", penwidth=1.9];\n",
                        nodes[i].id, nodes[j].id, EDGE_RIGHT);
        }
    }

    fprintf(dot, "}\n");
    fclose(dot);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "dot -T svg \"%s\" -o \"%s\"", dot_path, svg_path);
    system(cmd);

    FILE *html = fopen(filename, "a");
    if (html)
    {
        fprintf(html, "<h2>%s</h2>\n", title ? title : "Tree");
        fprintf(html, "<h3>Nodes: %zu</h3>\n", n);
        fprintf(html, "<h3>Root: 0x%p</h3>\n", (void*)root);
        fprintf(html, "<img src=\"temp/t%s\" />\n", svg_name);
        fprintf(html, "<hr/>\n");
        fclose(html);
    }

    free(nodes);
}

static const char* op_to_latex_func(node_operations_e op)
{
    switch (op)
    {
        case OP_SIN:   return "\\sin";
        case OP_COS:   return "\\cos";
        case OP_TAN:   return "\\tan";
        case OP_COT:   return "\\cot";

        case OP_SINH:  return "\\sinh";
        case OP_COSH:  return "\\cosh";
        case OP_TANH:  return "\\tanh";
        case OP_COTH:  return "\\coth";

        case OP_LN:    return "\\ln";
        case OP_SQRT:  return "\\sqrt";

        case OP_ASIN:  return "\\arcsin";
        case OP_ACOS:  return "\\arccos";
        case OP_ATAN:  return "\\arctan";
        case OP_ACOT:  return "\\operatorname{arccot}";

        default:       return NULL;
    }
}

static int need_parentheses(const node_t* node,
                            const node_t* parent,
                            int           is_right_child)
{
    if (!node || !parent)
        return 0;

    if (node->node_type != TYPE_OP || parent->node_type != TYPE_OP)
        return 0;

    size_t cr = node->rank;
    size_t pr = parent->rank;

    if (cr < pr)
        return 1;

    if (cr == pr)
    {
        switch (parent->value.op)
        {
            case OP_SUB:
            case OP_DIV:
            case OP_POW:
                if (is_right_child)
                    return 1;
                break;

            default:
                break;
        }
    }

    return 0;
}

static void latex_print_number(FILE* out, double v)
{
    double iv = floor(v);
    if (is_same(v, iv))
        fprintf(out, "%ld", (long)iv);
    else
        fprintf(out, "%.15g", v);
}

void latex_print_node(FILE* out,
                      const node_t* node,
                      const node_t* parent,
                      int           is_right_child)
{
    if (!node)
        return;

    int paren = need_parentheses(node, parent, is_right_child);
    if (paren)
        fprintf(out, "\\left(");

    if (node->node_type == TYPE_NUM)
    {
        latex_print_number(out, node->value.d_value);
    }
    else if (node->node_type == TYPE_VAR)
    {
        fprintf(out, "%s", node->value.var.name ? node->value.var.name : "");
    }
    else if (node->node_type == TYPE_OP)
    {
        node_operations_e op = node->value.op;
        const node_t* L = node->left;
        const node_t* R = node->right;

        switch (op)
        {
            case OP_ADD:
                latex_print_node(out, L, node, 0);
                fprintf(out, " + ");
                latex_print_node(out, R, node, 1);
                break;

            case OP_SUB:
                latex_print_node(out, L, node, 0);
                fprintf(out, " - ");
                latex_print_node(out, R, node, 1);
                break;

            case OP_MUL:
                latex_print_node(out, L, node, 0);
                fprintf(out, " \\cdot ");
                latex_print_node(out, R, node, 1);
                break;

            case OP_DIV:
                fprintf(out, "\\frac{");
                latex_print_node(out, L, node, 0);
                fprintf(out, "}{");
                latex_print_node(out, R, node, 1);
                fprintf(out, "}");
                break;

            case OP_POW:
            {
                int base_paren = 0;
                if (L && L->node_type == TYPE_OP)
                {
                    if (L->value.op == OP_ADD || L->value.op == OP_SUB)
                        base_paren = 1;
                }

                if (base_paren)
                    fprintf(out, "\\left(");

                latex_print_node(out, L, NULL, 0);
                if (base_paren)
                    fprintf(out, "\\right)");

                fprintf(out, "^{");
                latex_print_node(out, R, node, 1);
                fprintf(out, "}");
                break;
            }

            case OP_LOG:
                fprintf(out, "\\log_{");
                latex_print_node(out, L, node, 0);
                fprintf(out, "}");
                fprintf(out, "\\left(");
                latex_print_node(out, R, node, 1);
                fprintf(out, "\\right)");
                break;

            case OP_SQRT:
                fprintf(out, "\\sqrt{");
                latex_print_node(out, L, node, 0);
                fprintf(out, "}");
                break;

            case OP_LN:
            case OP_SIN:
            case OP_COS:
            case OP_TAN:
            case OP_COT:
            case OP_SINH:
            case OP_COSH:
            case OP_TANH:
            case OP_COTH:
            case OP_ASIN:
            case OP_ACOS:
            case OP_ATAN:
            case OP_ACOT:
            {
                const char* f = op_to_latex_func(op);
                if (!f)
                    f = "\\operatorname{f}";
                fprintf(out, "%s", f);
                fprintf(out, "\\left(");
                latex_print_node(out, L, node, 0);
                fprintf(out, "\\right)");
                break;
            }

            case OP_NOP:
            default:
                if (L)
                    latex_print_node(out, L, node, 0);
                break;
        }
    }

    if (paren)
        fprintf(out, "\\right)");
}

void tree_dump_begin(const char* filename)
{
    if (!CHECK(ERROR, filename != NULL, "tree_dump_begin: filename is NULL"))
        return;

    FILE* file = load_file(filename, "w");
    if (!CHECK(ERROR, file != NULL, "tree_dump_begin: load_file failed"))
        return;

    fprintf(file,
            "\\documentclass[a4paper,12pt]{article}\n"
            "\\usepackage[a4paper,top=1.3cm,bottom=2cm,left=1.5cm,right=1.5cm]{geometry}\n"
            "\\usepackage{amsmath,amsfonts,amssymb,amsthm,mathtools}\n"
            "\\usepackage{tikz}\n"
            "\\usepackage{pgfplots}\n"
            "\\pgfplotsset{compat=1.18}\n"
            "\\begin{document}\n\n");

    fclose(file);
}

void tree_dump_latex(const tree_t* tree,
                     const char*   filename,
                     const char*   comment)
{
    if (!CHECK(ERROR, tree != NULL,     "tree_dump_latex: tree is NULL"))
        return;
    if (!CHECK(ERROR, filename != NULL, "tree_dump_latex: filename is NULL"))
        return;

    if (!CHECK(ERROR, tree_verify(tree) == OK,
               "tree_dump_latex: tree verification failed"))
        return;

    FILE* file = load_file(filename, "a");
    if (!CHECK(ERROR, file != NULL, "tree_dump_latex: load_file failed"))
        return;

    if (comment && comment[0] != '\0')
    {
        fprintf(file, "%% %s\n", comment);
        fprintf(file, "\\noindent\\textbf{%s}\\\\[4pt]\n\n", comment);
    }

    fprintf(file, "\\begin{equation}\n");
    if (tree->root)
        latex_print_node(file, tree->root, NULL, 0);
    fprintf(file, "\n\\end{equation}\n\n");

    fclose(file);
}

void tree_dump_end(const char* filename)
{
    if (!CHECK(ERROR, filename != NULL, "tree_dump_end: filename is NULL"))
        return;

    FILE* file = load_file(filename, "a");
    if (!CHECK(ERROR, file != NULL, "tree_dump_end: load_file failed"))
        return;

    fprintf(file, "\\end{document}\n");
    fclose(file);

    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd),
             "pdflatex -interaction=nonstopmode -halt-on-error \"%s\" > /dev/null 2>&1",
             filename);
    unused system(cmd);
}

static double eval_node_plot(const node_t* node, double x)
{
    if (!node)
        return NAN;

    switch (node->node_type)
    {
        case TYPE_NUM:
            return node->value.d_value;

        case TYPE_VAR:
            if (node->value.var.name && strcmp(node->value.var.name, "x") == 0)
                return x;
            return 0.0;

        case TYPE_OP:
        {
            double left  = node->left  ? eval_node_plot(node->left,  x) : NAN;
            double right = node->right ? eval_node_plot(node->right, x) : NAN;

            switch (node->value.op)
            {
                case OP_ADD:  return left + right;
                case OP_SUB:  return left - right;
                case OP_MUL:  return left * right;
                case OP_DIV:  return right != 0.0 ? left / right : NAN;
                case OP_POW:  return pow(left, right);

                case OP_SIN:  return sin(left);
                case OP_COS:  return cos(left);
                case OP_TAN:  return tan(left);
                case OP_COT:  return 1.0 / tan(left);

                case OP_SINH: return sinh(left);
                case OP_COSH: return cosh(left);
                case OP_TANH: return tanh(left);
                case OP_COTH: return cosh(left) / sinh(left);

                case OP_LOG:  return log(right) / log(left);
                case OP_LN:   return log(left);
                case OP_SQRT: return left >= 0.0 ? sqrt(left) : NAN;

                case OP_ASIN: return asin(left);
                case OP_ACOS: return acos(left);
                case OP_ATAN: return atan(left);
                case OP_ACOT: return atan(1.0 / left);

                case OP_NOP:
                default:
                    return left;
            }
        }

        default:
            return NAN;
    }
}

void tree_dump_plot(const tree_t* tree,
                    const char*   filename,
                    double        x_from,
                    double        x_to,
                    double        y_from,
                    double        y_to,
                    double        step,
                    const char*   comment)
{
    if (!CHECK(ERROR, tree != NULL,     "tree_dump_plot: tree is NULL"))
        return;
    if (!CHECK(ERROR, filename != NULL, "tree_dump_plot: filename is NULL"))
        return;

    if (!CHECK(ERROR, tree_verify(tree) == OK,
               "tree_dump_plot: tree verification failed"))
        return;

    if (step <= 0.0 || x_to <= x_from)
        return;

    FILE* file = load_file(filename, "a");
    if (!CHECK(ERROR, file != NULL, "tree_dump_plot: load_file failed"))
        return;

    if (comment && comment[0] != '\0')
    {
        fprintf(file, "%% %s\n", comment);
        fprintf(file, "\\noindent\\textbf{%s}\\\\[4pt]\n\n", comment);
    }

    fprintf(file,
            "\\begin{tikzpicture}\n"
            "\\begin{axis}[\n"
            "  xmin=%g, xmax=%g,\n"
            "  ymin=%g, ymax=%g,\n"
            "  axis lines=middle,\n"
            "  grid=both,\n"
            "  xlabel={$x$},\n"
            "  ylabel={$y$}\n"
            "]\n",
            x_from, x_to, y_from, y_to);

    fprintf(file, "\\addplot[smooth] coordinates {\n");

    for (double x = x_from; x <= x_to + 0.5 * step; x += step)
    {
        double y = eval_node_plot(tree->root, x);
        if (!isnan(y) && !isinf(y))
            fprintf(file, "  (%g,%g)\n", x, y);
    }

    fprintf(file,
            "};\n"
            "\\end{axis}\n"
            "\\end{tikzpicture}\n\n");

    fclose(file);
}
