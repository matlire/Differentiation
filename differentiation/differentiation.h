#ifndef DIFFERENTIATION_H
#define DIFFERENTIATION_H

#include "../tree/tree.h"
#include "../tree/dump/dump.h"

err_t tree_derivative(tree_t* in_tree, tree_t* out_tree, const char * const variable, size_t n);

#endif
