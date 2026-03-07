#include "options.h"
#include "strlist.h"
#include "walker.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    DirwalkOptions opts;
    StrList paths;
    int walk_result;
    size_t i;

    if (parse_options(argc, argv, &opts) != 0) {
        return EXIT_FAILURE;
    }

    strlist_init(&paths);
    if (opts.sort_output) {
        if (!setlocale(LC_COLLATE, "")) {
            fprintf(stderr, "dirwalk: warning: cannot apply locale collation\n");
        }
    }

    walk_result = walk_tree(opts.start_dir, &opts, opts.sort_output ? &paths : NULL);
    if (walk_result == -1) {
        strlist_free(&paths);
        return EXIT_FAILURE;
    }

    if (opts.sort_output) {
        strlist_sort_locale(&paths);
        for (i = 0; i < paths.size; ++i) {
            if (puts(paths.items[i]) == EOF) {
                strlist_free(&paths);
                return EXIT_FAILURE;
            }
        }
    }

    strlist_free(&paths);
    return walk_result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
