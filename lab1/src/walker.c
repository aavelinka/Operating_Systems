#include "walker.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    const DirwalkOptions *opts;
    StrList *output;
    int had_access_errors;
    int output_failed;
} WalkState;

static char *join_path(const char *base, const char *name)
{
    size_t base_len = strlen(base);
    size_t name_len = strlen(name);
    int needs_slash = (base_len > 0 && base[base_len - 1] != '/');
    size_t total_len = base_len + (size_t)needs_slash + name_len + 1;
    char *result = malloc(total_len);

    if (!result) {
        return NULL;
    }

    memcpy(result, base, base_len);
    if (needs_slash) {
        result[base_len] = '/';
        memcpy(result + base_len + 1, name, name_len);
        result[base_len + 1 + name_len] = '\0';
    } else {
        memcpy(result + base_len, name, name_len);
        result[base_len + name_len] = '\0';
    }

    return result;
}

static int matches_type(mode_t mode, const DirwalkOptions *opts)
{
    int no_filter = !opts->want_links && !opts->want_dirs && !opts->want_files;

    if (no_filter) {
        return S_ISLNK(mode) || S_ISDIR(mode) || S_ISREG(mode);
    }

    if (opts->want_links && S_ISLNK(mode)) {
        return 1;
    }
    if (opts->want_dirs && S_ISDIR(mode)) {
        return 1;
    }
    if (opts->want_files && S_ISREG(mode)) {
        return 1;
    }
    return 0;
}

static int emit_path(const char *path, WalkState *state)
{
    if (state->output) {
        if (strlist_push(state->output, path) != 0) {
            fprintf(stderr, "dirwalk: out of memory\n");
            return -1;
        }
        return 0;
    }

    if (puts(path) == EOF) {
        state->output_failed = 1;
        return -1;
    }
    return 0;
}

static int walk_node(const char *path, WalkState *state)
{
    struct stat st;
    DIR *dir;
    struct dirent *entry;

    if (lstat(path, &st) != 0) {
        fprintf(stderr, "dirwalk: cannot access '%s': %s\n", path, strerror(errno));
        state->had_access_errors = 1;
        return 0;
    }

    if (matches_type(st.st_mode, state->opts)) {
        if (emit_path(path, state) != 0) {
            return -1;
        }
    }

    if (!S_ISDIR(st.st_mode)) {
        return 0;
    }

    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "dirwalk: cannot open directory '%s': %s\n", path, strerror(errno));
        state->had_access_errors = 1;
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        char *child_path;
        int rc;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        child_path = join_path(path, entry->d_name);
        if (!child_path) {
            fprintf(stderr, "dirwalk: out of memory\n");
            closedir(dir);
            return -1;
        }

        rc = walk_node(child_path, state);
        free(child_path);
        if (rc != 0) {
            closedir(dir);
            return rc;
        }
    }

    if (closedir(dir) != 0) {
        fprintf(stderr, "dirwalk: cannot close directory '%s': %s\n", path, strerror(errno));
        state->had_access_errors = 1;
    }

    return 0;
}

int walk_tree(const char *root, const DirwalkOptions *opts, StrList *output)
{
    WalkState state;
    int rc;

    state.opts = opts;
    state.output = output;
    state.had_access_errors = 0;
    state.output_failed = 0;

    rc = walk_node(root, &state);
    if (rc != 0) {
        return -1;
    }
    if (state.output_failed) {
        return -1;
    }
    return state.had_access_errors ? 1 : 0;
}
