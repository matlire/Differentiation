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

    derivative_config_t config = {  };
    
    CREATE_TREE(tree);
    unused tree_read_file(&tree, &config, "eq.db", ERROR);
    CREATE_TREE(dtree);
    unused tree_derivative(&tree, &dtree, &config);
    tree_dtor(&tree);
    tree_dtor(&dtree);
    
    close_log_file();
    return 0;
}
