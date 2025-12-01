#include "libs/types.h"
#include <locale.h>
#include "libs/types.h"
#include "tree/tree.h"
#include "tree/dump/dump.h"
#include "differentiation/differentiation.h"

const char* DEFAULT_DB_FILE = "diff3.db";

int main()
{
    setlocale(LC_ALL, "");
    init_logging("log.log", DEBUG);

    //const char* s = "10*(30+20*10)+16/(3+1)$";
    //err_t ret = OK;

    CREATE_TREE(tree1);
    unused tree_read_file(&tree1, DEFAULT_DB_FILE, ERROR);    
    printf("%lf", evaluate_tree(&tree1));
    tree_dump_latex(&tree1, "tree1.tex");

    tree_dtor(&tree1);

    /*
    CREATE_TREE(tree1);
    unused tree_read_file(&tree1, DEFAULT_DB_FILE, ERROR);
    tree_dump_graphviz(&tree1, "tree1", "tgdump.html");
    tree_dump_latex(&tree1, "tree1.tex");
    printf("%lf", evaluate_tree(&tree1));
    unused tree_fprint("out1.db", &tree1);
    tree_dtor(&tree1);
    
    CREATE_TREE(tree2);
    unused tree_read_file(&tree2, "diff2.db", ERROR);
    tree_dump_graphviz(&tree2, "tree2", "tgdump.html");
    tree_dump_latex(&tree2, "2.tex");
    CREATE_TREE(dtree2);
    unused tree_derivative(&tree2, &dtree2, "x", 1);
    tree_optimize(&dtree2);
    tree_dump_latex(&dtree2, "d2.tex");
    unused tree_fprint("out2.db", &tree2);
    tree_dtor(&tree2);
    tree_dtor(&dtree2);
    */
    
    close_log_file();
    return 0;
}
