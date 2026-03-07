#ifndef ENV_UTILS_H
#define ENV_UTILS_H

#include <stddef.h>

int print_sorted_environment(char *const envp[]);
int load_env_names(const char *path, char ***names_out, size_t *count_out);
char **build_reduced_env(char *const source_env[], char *const names[], size_t names_count);
void free_string_array(char **arr, size_t count);
void free_envp(char **envp);

#endif
