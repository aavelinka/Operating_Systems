#include "options.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *prog_name, FILE *out)
{
    fprintf(out, "Usage: %s [dir] [options]\n", prog_name);
    fprintf(out, "Options:\n");
    fprintf(out, "  -l    only symbolic links\n");
    fprintf(out, "  -d    only directories\n");
    fprintf(out, "  -f    only regular files\n");
    fprintf(out, "  -s    sort output by LC_COLLATE\n");
}

int parse_options(int argc, char *argv[], DirwalkOptions *opts)
{
    int i, c;
    int after_dashdash = 0;
    int option_argc = 1;
    const char *start_dir = NULL;
    char **option_argv;

    opts->want_links = 0;
    opts->want_dirs = 0;
    opts->want_files = 0;
    opts->sort_output = 0;
    opts->start_dir = "./";

    option_argv = malloc((size_t)(argc + 1) * sizeof(*option_argv));
    if (!option_argv) {
        fprintf(stderr, "Error: out of memory while parsing options.\n");
        return -1;
    }

    option_argv[0] = argv[0];

    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (!after_dashdash && strcmp(arg, "--") == 0) {
            after_dashdash = 1;
            continue;
        }

        if (!after_dashdash && arg[0] == '-' && arg[1] != '\0') {
            option_argv[option_argc++] = argv[i];
            continue;
        }

        if (!start_dir) {
            start_dir = arg;
        } else {
            fprintf(stderr, "Error: only one start directory is allowed.\n");
            free(option_argv);
            return -1;
        }
    }

    opterr = 0;
    optind = 1;
    while ((c = getopt(option_argc, option_argv, "ldfs")) != -1) {
        switch (c) {
        case 'l':
            opts->want_links = 1;
            break;
        case 'd':
            opts->want_dirs = 1;
            break;
        case 'f':
            opts->want_files = 1;
            break;
        case 's':
            opts->sort_output = 1;
            break;
        default:
            fprintf(stderr, "Error: unknown option '-%c'.\n", optopt ? optopt : '?');
            print_usage(argv[0], stderr);
            free(option_argv);
            return -1;
        }
    }

    if (start_dir) {
        opts->start_dir = start_dir;
    }

    free(option_argv);
    return 0;
}
