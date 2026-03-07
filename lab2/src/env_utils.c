#include "env_utils.h"

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int env_cmp(const void *lhs, const void *rhs) { 
  const char *const *left = lhs;
  const char *const *right = rhs;
  return strcoll(*left, *right);
}

int print_sorted_environment(char *const envp[]) {
  size_t count = 0;
  char **copy = NULL;

  if (setlocale(LC_COLLATE, "C") == NULL) {
    fprintf(stderr, "warning: failed to set LC_COLLATE=C, using current locale\n");
  }

  while (envp[count] != NULL) {
    count++;
  }

  copy = malloc(count * sizeof(*copy));
  if (copy == NULL) {
    perror("malloc");
    return -1;
  }

  for (size_t i = 0; i < count; i++) {
    copy[i] = envp[i];
  }

  qsort(copy, count, sizeof(*copy), env_cmp);

  for (size_t i = 0; i < count; i++) {
    puts(copy[i]);
  }

  free(copy);
  return 0;
}

static char *trim_left(char *s) { 
  while (*s != '\0' && isspace((unsigned char)*s)) {
    s++;
  }
  return s;
}

static void trim_right(char *s) {
  size_t len = strlen(s); 
  while (len > 0 && isspace((unsigned char)s[len - 1])) {
    s[len - 1] = '\0';
    len--;
  }
}

static int is_valid_name(const char *name) {
  if (name[0] == '\0') {
    return 0;
  }
  if (!(isalpha((unsigned char)name[0]) || name[0] == '_')) {
    return 0;
  }

  for (size_t i = 1; name[i] != '\0'; i++) {
    if (!(isalnum((unsigned char)name[i]) || name[i] == '_')) {
      return 0;
    }
  }

  return 1;
}

int load_env_names(const char *path, char ***names_out, size_t *count_out) {
  FILE *file = NULL;
  char **names = NULL;
  size_t count = 0;
  size_t cap = 0;
  char line[1024];

  file = fopen(path, "r");
  if (file == NULL) {
    perror("fopen");
    return -1;
  }

  while (fgets(line, sizeof(line), file) != NULL) {
    char *name = NULL;
    char *entry = NULL;

    trim_right(line);
    name = trim_left(line);

    if (name[0] == '\0' || name[0] == '#') {
      continue;
    }

    if (!is_valid_name(name)) {
      fprintf(stderr, "warning: invalid variable name in env file: '%s'\n", name);
      continue;
    }

    if (count == cap) {
      size_t new_cap = (cap == 0) ? 8 : (cap * 2);
      char **tmp = realloc(names, new_cap * sizeof(*tmp));
      if (tmp == NULL) {
        perror("realloc");
        free_string_array(names, count);
        fclose(file);
        return -1;
      }
      names = tmp;
      cap = new_cap;
    }

    entry = malloc(strlen(name) + 1);
    if (entry == NULL) {
      perror("malloc");
      free_string_array(names, count);
      fclose(file);
      return -1;
    }
    strcpy(entry, name);

    names[count] = entry;
    count++;
  }

  if (ferror(file)) {
    perror("fgets");
    free_string_array(names, count);
    fclose(file);
    return -1;
  }

  fclose(file);
  *names_out = names;
  *count_out = count;
  return 0;
}

static const char *find_env_value(char *const source_env[], const char *name) {
  size_t name_len = strlen(name);

  for (size_t i = 0; source_env[i] != NULL; i++) {
    if (strncmp(source_env[i], name, name_len) == 0 && source_env[i][name_len] == '=') {
      return source_env[i] + name_len + 1;
    }
  }

  return NULL;
}

char **build_reduced_env(char *const source_env[], char *const names[], size_t names_count) {
  char **envp = NULL;

  envp = calloc(names_count + 1, sizeof(*envp));
  if (envp == NULL) {
    perror("calloc");
    return NULL;
  }

  for (size_t i = 0; i < names_count; i++) {
    const char *value = find_env_value(source_env, names[i]);
    size_t name_len = strlen(names[i]);
    size_t value_len = (value == NULL) ? 0 : strlen(value);
    char *entry = malloc(name_len + 1 + value_len + 1);

    if (entry == NULL) {
      perror("malloc");
      free_envp(envp);
      return NULL;
    }

    memcpy(entry, names[i], name_len);
    entry[name_len] = '=';
    if (value != NULL) {
      memcpy(entry + name_len + 1, value, value_len);
    }
    entry[name_len + 1 + value_len] = '\0';

    envp[i] = entry;
  }

  envp[names_count] = NULL;
  return envp;
}

void free_string_array(char **arr, size_t count) {
  if (arr == NULL) {
    return;
  }

  for (size_t i = 0; i < count; i++) {
    free(arr[i]);
  }
  free(arr);
}

void free_envp(char **envp) {
  size_t i = 0;

  if (envp == NULL) {
    return;
  }

  while (envp[i] != NULL) {
    free(envp[i]);
    i++;
  }
  free(envp);
}
