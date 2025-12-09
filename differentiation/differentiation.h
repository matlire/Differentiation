#ifndef DIFFERENTIATION_H
#define DIFFERENTIATION_H

#include "../tree/tree.h"
#include "../tree/dump/dump.h"
#include "../libs/io/io.h"

err_t tree_derivative(tree_t* in_tree,
                      tree_t* out_tree,
                      derivative_config_t* config);

#endif
