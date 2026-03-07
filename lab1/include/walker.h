#ifndef WALKER_H
#define WALKER_H

#include "options.h"
#include "strlist.h"

int walk_tree(const char *root, const DirwalkOptions *opts, StrList *output);

#endif
