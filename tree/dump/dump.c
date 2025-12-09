#include "dump.h"

err_t tree_derivative(tree_t* in_tree,
                      tree_t* out_tree,
                      derivative_config_t* config);

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
            const char* op_str = op_to_str(node->value.op);
            snprintf(type_buf,  type_buf_size,  "OP");
            snprintf(value_buf, value_buf_size,
                     "%s (id=%d)", op_str ? op_str : "?", (int)node->value.op);
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

#define MAX_MEMES    64
#define MAX_MEME_LEN 256

static char*  g_memes[MAX_MEMES] = { 0 };
static size_t g_memes_count      = 0;
static int    g_memes_loaded     = 0;

static void memes_load(void)
{
    if (g_memes_loaded)
        return;

    FILE* f = load_file("memes.txt", "r");
    if (!f)
    {
        g_memes_loaded = 1;
        return;
    }

    char buf[MAX_MEME_LEN] = { 0 };

    while (g_memes_count < MAX_MEMES && fgets(buf, sizeof(buf), f))
    {
        size_t len = strlen(buf);
        while (len > 0 &&
               (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
            buf[--len] = '\0';

        if (len == 0)
            continue;

        char* line = (char*)calloc(len + 1, 1);
        if (!line)
            break;

        memcpy(line, buf, len);
        line[len] = '\0';

        g_memes[g_memes_count++] = line;
    }

    fclose(f);
    g_memes_loaded = 1;
}

static const char* memes_get_random(void)
{
    static int seeded = 0;
    if (!seeded)
    {
        srand((unsigned)time(NULL));
        seeded = 1;
    }

    memes_load();

    if (g_memes_count == 0)
        return NULL;

    size_t idx = (size_t)(rand() % (int)g_memes_count);
    return g_memes[idx];
}

static void dump_heading_internal(const char* filename, const char* text)
{
    if (!filename || !text)
        return;

    FILE* f = load_file(filename, "a");
    if (!f)
        return;

    fprintf(f, "\\noindent\\textbf{%s}\\\\[6pt]\n\n", text);
    fclose(f);
}

static void dump_meme_internal(const char* filename, const char* meme)
{
    if (!filename || !meme || meme[0] == '\0')
        return;

    FILE* f = load_file(filename, "a");
    if (!f)
        return;

    fprintf(f, "\\noindent\\textit{%s}\\\\[4pt]\n\n", meme);
    fclose(f);
}

void tree_dump_heading(derivative_config_t* config,
                       const char*          text)
{
    if (!config || !config->dump_filename || !text)
        return;

    dump_heading_internal(config->dump_filename, text);
}

void tree_dump_random_meme(derivative_config_t* config)
{
    if (!config || !config->dump_filename)
        return;

    const char* meme = memes_get_random();
    if (!meme)
        return;

    dump_meme_internal(config->dump_filename, meme);
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

void tree_dump_graphviz(const tree_t *tree, derivative_config_t* config)
{
    if (!tree || !config || !config->dump_filename) return;

    const node_t *root = tree->root;
    const char* filename = config->dump_filename;

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
            fprintf(html, "<h2>%s</h2>\n", "Tree");
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

        char type_str[32]   = {0};
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
        fprintf(html, "<h2>%s</h2>\n", "Tree");
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
                fprintf(out, " + \\allowbreak ");
                latex_print_node(out, R, node, 1);
                break;

            case OP_SUB:
                latex_print_node(out, L, node, 0);
                fprintf(out, " - \\allowbreak ");
                latex_print_node(out, R, node, 1);
                break;

            case OP_MUL:
                latex_print_node(out, L, node, 0);
                fprintf(out, " \\cdot \\allowbreak ");
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
                int base_paren = (L && L->node_type == TYPE_OP);

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
                latex_print_node(out, R, NULL, 0);
                fprintf(out, "\\right)");
                break;

            case OP_SQRT:
                fprintf(out, "\\sqrt{");
                latex_print_node(out, L, NULL, 0);
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
                latex_print_node(out, L, NULL, 0);
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

static double eval_node_plot(const node_t* node, char var_name, double x)
{
    if (!node)
        return NAN;

    switch (node->node_type)
    {
        case TYPE_NUM:
            return node->value.d_value;

        case TYPE_VAR:
            if (node->value.var.name &&
                node->value.var.name[0] == var_name &&
                node->value.var.name[1] == '\0')
                return x;
            return node->value.var.value;

        case TYPE_OP:
        {
            double left  = node->left  ? eval_node_plot(node->left,  var_name, x) : NAN;
            double right = node->right ? eval_node_plot(node->right, var_name, x) : NAN;

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

static FILE* dump_open_append(const tree_t*        tree,
                              derivative_config_t* config,
                              int                  need_tree_verify,
                              const char*          who)
{
    unused(who);

    if (!CHECK(ERROR, config != NULL, "dump_open_append: config is NULL"))
        return NULL;
    if (!CHECK(ERROR, config->dump_filename != NULL, "dump_open_append: dump_filename is NULL"))
        return NULL;

    if (need_tree_verify)
    {
        if (!CHECK(ERROR, tree != NULL, "dump_open_append: tree is NULL"))
            return NULL;
        if (!CHECK(ERROR, tree_verify(tree) == OK,
                   "dump_open_append: tree verification failed"))
            return NULL;
    }

    FILE* file = load_file(config->dump_filename, "a");
    if (!CHECK(ERROR, file != NULL, "dump_open_append: load_file failed"))
        return NULL;

    return file;
}

typedef struct AxisParams
{
    double x_from;
    double x_to;
    double y_from;
    double y_to;
    int    has_x;
    int    has_y;
    int    restrict_y;
} AxisParams;

static AxisParams axis_params_from_config(derivative_config_t* config,
                                          int use_taylor_y)
{
    AxisParams p;
    p.x_from = config->x_from;
    p.x_to   = config->x_to;
    if (use_taylor_y)
    {
        p.y_from = config->taylor_y_from;
        p.y_to   = config->taylor_y_to;
    }
    else
    {
        p.y_from = config->y_from;
        p.y_to   = config->y_to;
    }
    p.has_x = !(p.x_from == 0.0 && p.x_to == 0.0);
    p.has_y = !(p.y_from == 0.0 && p.y_to == 0.0);
    p.restrict_y = p.has_x && p.has_y;
    return p;
}

static void dump_axis_begin(FILE* file, const AxisParams* p)
{
    fprintf(file,
            "\\begin{tikzpicture}\n"
            "\\begin{axis}[\n");

    if (p->has_x)
        fprintf(file, "  xmin=%g, xmax=%g,\n", p->x_from, p->x_to);
    if (p->has_y)
        fprintf(file, "  ymin=%g, ymax=%g,\n", p->y_from, p->y_to);

    fprintf(file,
            "  axis lines=middle,\n"
            "  grid=both,\n"
            "  xlabel={$x$},\n"
            "  ylabel={$y$}");

    if (p->restrict_y)
        fprintf(file,
                ",\n"
                "  restrict y to domain=%g:%g,\n"
                "  unbounded coords=discard",
                p->y_from, p->y_to);

    fprintf(file, "\n]\n");
}

static void dump_axis_end(FILE* file)
{
    fprintf(file,
            "\\end{axis}\n"
            "\\end{tikzpicture}\n\n");
}

static void dump_function_curve(FILE* file,
                                const node_t* root,
                                char          var_name,
                                double        x_from,
                                double        x_to,
                                double        step)
{
    fprintf(file, "\\addplot[smooth] coordinates {\n");
    for (double x = x_from; x <= x_to + 0.5 * step; x += step)
    {
        double y = eval_node_plot(root, var_name, x);
        if (!isnan(y) && !isinf(y))
            fprintf(file, "  (%g,%g)\n", x, y);
    }
    fprintf(file, "};\n");
}

void tree_dump_begin(derivative_config_t* config)
{
    if (!CHECK(ERROR, config != NULL, "tree_dump_begin: config is NULL"))
        return;
    if (!CHECK(ERROR, config->dump_filename != NULL, "tree_dump_begin: dump_filename is NULL"))
        return;

    FILE* file = load_file(config->dump_filename, "w");
    if (!CHECK(ERROR, file != NULL, "tree_dump_begin: load_file failed"))
        return;

    fprintf(file,
            "\\documentclass[a4paper,12pt]{article}\n"
            "\\usepackage[T2A]{fontenc}\n"
            "\\usepackage[utf8]{inputenc}\n"
            "\\usepackage[english,russian]{babel}\n"
            "\\usepackage[a4paper,top=1.3cm,bottom=2cm,left=1.5cm,right=1.5cm]{geometry}\n"
            "\\usepackage{amsmath,amsfonts,amssymb,amsthm,mathtools}\n"
            "\\usepackage{breqn}\n"
            "\\breqnsetup{breakdepth=1}\n"
            "\\usepackage{tikz}\n"
            "\\usepackage{pgfplots}\n"
            "\\usepackage{hyperref}\n"
            "\\pgfplotsset{compat=1.18}\n"
            "\\begin{document}\n\n"
            "\\tableofcontents\n"
            "\\newpage\n\n");

    fclose(file);
}

void tree_dump_latex(const tree_t* tree,
                     derivative_config_t* config,
                     const char*   comment)
{
    FILE* file = dump_open_append(tree, config, 1, "tree_dump_latex");
    if (!file)
        return;

    if (comment && comment[0] != '\0')
    {
        fprintf(file, "%% %s\n", comment);
        if (strcmp(comment, "Перед оптимизацией:") != 0 &&
            strcmp(comment, "После оптимизации:") != 0)
        {
            fprintf(file,
                    "\\section*{%s}\n"
                    "\\addcontentsline{toc}{section}{%s}\n\n",
                    comment, comment);
        }
        else
        {
            fprintf(file, "\n");
        }
    }

    fprintf(file, "\\begin{dmath}\n");
    if (tree->root)
        latex_print_node(file, tree->root, NULL, 0);
    fprintf(file, "\n\\end{dmath}\n\n");

    fclose(file);
}

void tree_dump_end(derivative_config_t* config)
{
    if (!CHECK(ERROR, config != NULL, "tree_dump_end: config is NULL"))
        return;
    if (!CHECK(ERROR, config->dump_filename != NULL, "tree_dump_end: dump_filename is NULL"))
        return;

    FILE* file = load_file(config->dump_filename, "a");
    if (!CHECK(ERROR, file != NULL, "tree_dump_end: load_file failed"))
        return;

    fprintf(file, "\\end{document}\n");
    fclose(file);

    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd),
             "pdflatex -interaction=nonstopmode -halt-on-error \"%s\" > /dev/null 2>&1",
             config->dump_filename);
    unused system(cmd);
    unused system(cmd);
}

void tree_dump_plot(const tree_t* tree,
                    derivative_config_t* config,
                    const char*   comment)
{
    FILE* file = dump_open_append(tree, config, 1, "tree_dump_plot");
    if (!file)
        return;

    if (comment && comment[0] != '\0')
    {
        fprintf(file, "%% %s\n", comment);
        fprintf(file, "\\noindent\\textbf{%s}\\\\[4pt]\n\n", comment);
    }

    double step = config->step;
    if (step <= 0.0)
        step = 0.01;

    AxisParams p = axis_params_from_config(config, 0);
    dump_axis_begin(file, &p);
    dump_function_curve(file, tree->root, config->variable,
                        config->x_from, config->x_to, step);
    dump_axis_end(file);

    fclose(file);
}

void tree_dump_plot_tangent(const tree_t* tree,
                            derivative_config_t* config,
                            const char*   comment)
{
    FILE* file = dump_open_append(tree, config, 1, "tree_dump_plot_tangent");
    if (!file)
        return;

    if (comment && comment[0] != '\0')
    {
        fprintf(file,
                "\\section*{%s}\n"
                "\\addcontentsline{toc}{section}{%s}\n\n",
                comment, comment);
    }

    double step = config->step;
    if (step <= 0.0)
        step = 0.01;

    AxisParams p = axis_params_from_config(config, 0);
    dump_axis_begin(file, &p);

    dump_function_curve(file, tree->root, config->variable,
                        config->x_from, config->x_to, step);

    double x0 = config->tangent_x;
    double y0 = eval_node_plot(tree->root, config->variable, x0);
    double h  = step;
    if (h <= 0.0)
        h = 0.01;
    double y_plus  = eval_node_plot(tree->root, config->variable, x0 + h);
    double y_minus = eval_node_plot(tree->root, config->variable, x0 - h);
    double m = NAN;

    if (!isnan(y_plus) && !isinf(y_plus) &&
        !isnan(y_minus) && !isinf(y_minus))
    {
        m = (y_plus - y_minus) / (2.0 * h);
    }

    if (!isnan(y0) && !isinf(y0) && !isnan(m) && !isinf(m))
    {
        fprintf(file, "\\addplot[smooth, color=red] coordinates {\n");
        for (double x = config->x_from; x <= config->x_to + 0.5 * step; x += step)
        {
            double yt = y0 + m * (x - x0);
            if (!isnan(yt) && !isinf(yt))
                fprintf(file, "  (%g,%g)\n", x, yt);
        }
        fprintf(file, "};\n");
    }

    dump_axis_end(file);
    fclose(file);
}

static int compute_taylor_coeffs(const tree_t* tree,
                                 derivative_config_t* base_config,
                                 size_t n,
                                 double* coeffs)
{
    if (!tree || !tree->root || !coeffs || !base_config)
        return 0;

    double x0 = base_config->tangent_x;
    double f0 = eval_node_plot(tree->root, base_config->variable, x0);
    if (isnan(f0) || isinf(f0))
        return 0;

    coeffs[0] = f0;
    double fact = 1.0;

    for (size_t k = 1; k <= n; ++k)
    {
        tree_t d;
        if (tree_ctor(&d) != OK)
            return 0;

        derivative_config_t cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.dump_filename = NULL;
        cfg.derivative_n  = k;
        cfg.variable      = base_config->variable;

        if (tree_derivative((tree_t*)tree, &d, &cfg) != OK)
        {
            tree_dtor(&d);
            return 0;
        }

        double dk = eval_node_plot(d.root, base_config->variable, x0);
        tree_dtor(&d);

        if (isnan(dk) || isinf(dk))
            return 0;

        fact *= (double)k;
        coeffs[k] = dk / fact;
    }

    return 1;
}

static double eval_taylor_from_coeffs(const double* coeffs,
                                      size_t n,
                                      double x,
                                      double x0)
{
    double dx = x - x0;
    double res = coeffs[0];
    double p = 1.0;

    for (size_t k = 1; k <= n; ++k)
    {
        p *= dx;
        res += coeffs[k] * p;
    }

    return res;
}

void tree_dump_plot_taylor(const tree_t* tree,
                           derivative_config_t* config,
                           const char*   comment)
{
    FILE* file = dump_open_append(tree, config, 1, "tree_dump_plot_taylor");
    if (!file)
        return;

    if (comment && comment[0] != '\0')
    {
        fprintf(file,
                "\\section*{%s}\n"
                "\\addcontentsline{toc}{section}{%s}\n\n",
                comment, comment);
    }

    double step = config->step;
    if (step <= 0.0)
        step = 0.01;

    AxisParams p = axis_params_from_config(config, 1);
    dump_axis_begin(file, &p);

    dump_function_curve(file, tree->root, config->variable,
                        config->x_from, config->x_to, step);

    size_t n = config->taylor_n;
    double x0 = 0.0;

    if (n > 0)
    {
        double* coeffs = (double*)calloc(n + 1, sizeof(double));
        if (coeffs)
        {
            if (compute_taylor_coeffs(tree, config, n, coeffs))
            {
                fprintf(file, "\\addplot[smooth, color=red] coordinates {\n");
                for (double x = config->x_from;
                     x <= config->x_to + 0.5 * step;
                     x += step)
                {
                    double y = eval_taylor_from_coeffs(coeffs, n, x, x0);
                    if (!isnan(y) && !isinf(y))
                        fprintf(file, "  (%g,%g)\n", x, y);
                }
                fprintf(file, "};\n");
            }
            free(coeffs);
        }
    }

    dump_axis_end(file);
    fclose(file);
}

