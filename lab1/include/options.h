#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>

typedef struct {
    int want_links;
    int want_dirs;
    int want_files;
    int sort_output;
    const char *start_dir;
} DirwalkOptions;

int parse_options(int argc, char *argv[], DirwalkOptions *opts);
void print_usage(const char *prog_name, FILE *out);

#endif
