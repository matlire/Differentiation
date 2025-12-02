#include "libs/types.h"
#include <locale.h>
#include "libs/types.h"
#include "tree/tree.h"
#include "tree/dump/dump.h"
#include "differentiation/differentiation.h"

int main()
{
    setlocale(LC_ALL, "");
    init_logging("log.log", DEBUG);

    CREATE_TREE(tree1);
    unused tree_read_file(&tree1, "diff2.db", ERROR);    
    tree_dtor(&tree1);
    
    CREATE_TREE(tree2);
    unused tree_read_file(&tree2, "diff2.db", ERROR);
    tree_dump_graphviz(&tree2, "tree2", "tgdump.html");
    CREATE_TREE(dtree2);
    unused tree_derivative(&tree2, &dtree2, "x", 1);
    tree_optimize(&dtree2);
    tree_dump_reset("d3.tex");
    tree_dump_begin("d3.tex");
    tree_dump_latex(&dtree2, "d3.tex", "First derivative of equation");
    tree_dump_plot(&dtree2, "d3.tex", -10, 10, -10, 10, 0.1, "Plot dump");
    tree_dump_end("d3.tex");
    tree_dtor(&tree2);
    tree_dtor(&dtree2);
    
    close_log_file();
    return 0;
}
