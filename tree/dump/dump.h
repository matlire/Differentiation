#ifndef TDUMP_H
#define TDUMP_H

#include "../tree.h"
#include "../../libs/logging/logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void tree_dump_reset   (const char* filename);
void tree_dump_graphviz(const tree_t* tree, const char* title, const char* filename);
void tree_dump_latex   (const tree_t* tree, const char* filename);

#endif
