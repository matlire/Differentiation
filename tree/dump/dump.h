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

void tree_dump_reset (const char* filename); 
void tree_dump_graphviz(const tree_t* tree, derivative_config_t* config);

void tree_dump_begin(derivative_config_t* config); 
void tree_dump_latex(const tree_t* tree, derivative_config_t* config, const char* comment); 
void tree_dump_end (derivative_config_t* config); 

void tree_dump_heading(derivative_config_t* config,
                       const char*          text);

void tree_dump_random_meme(derivative_config_t* config);

void tree_dump_plot(const tree_t* tree, derivative_config_t* config, const char* comment); 
void tree_dump_plot_tangent(const tree_t* tree, derivative_config_t* config, const char* comment);
void tree_dump_plot_taylor(const tree_t* tree, derivative_config_t* config, const char* comment);

#endif
