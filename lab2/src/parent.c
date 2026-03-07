#include "env_utils.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ; 

static void reap_children_nonblocking(void) {
  int status = 0;
  pid_t pid = 0;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    (void)pid;
    (void)status;
  }
}

static void wait_all_children(void) {
  int status = 0;
  pid_t pid = 0;

  for (;;) {
    pid = wait(&status);
    if (pid > 0) {
      continue;
    }
    if (pid == -1 && errno == EINTR) {
      continue;
    }
    if (pid == -1 && errno == ECHILD) {
      break;
    }
    if (pid == -1) {
      perror("wait");
      break;
    }
  }
}

static int build_child_path(char *buffer, size_t size) {
  const char *dir = getenv("CHILD_PATH");
  int written = 0;
  size_t dir_len = 0;
  const char *suffix = "/child";

  if (dir == NULL || dir[0] == '\0') {
    dir = ".";
  }

  dir_len = strlen(dir);
  if (dir_len > 0 && dir[dir_len - 1] == '/') {
    suffix = "child";
  }

  written = snprintf(buffer, size, "%s%s", dir, suffix);
  if (written < 0 || (size_t)written >= size) {
    fprintf(stderr, "child path is too long\n");
    return -1;
  }

  return 0;
}

static int spawn_child(const char *child_path, char mode, unsigned index, char *const child_env[]) {
  char child_name[16];
  char mode_arg[2];
  pid_t pid = 0;

  snprintf(child_name, sizeof(child_name), "child_%02u", index % 100U);
  mode_arg[0] = mode;
  mode_arg[1] = '\0';

  pid = fork();
  if (pid < 0) {
    perror("fork");
    return -1;
  }

  if (pid == 0) {
    char *argv[] = {child_name, mode_arg, NULL};
    execve(child_path, argv, child_env);
    perror("execve");
    _exit(127); 
  }

  return 0;
}

int main(void) {
  const char *env_file = "env";
  char child_path[PATH_MAX];
  char **env_names = NULL;
  size_t env_name_count = 0;
  char **child_env = NULL;
  unsigned child_index = 0;
  int ch = 0;

  if (print_sorted_environment(environ) != 0) {
    return EXIT_FAILURE;
  }

  if (load_env_names(env_file, &env_names, &env_name_count) != 0) {
    return EXIT_FAILURE;
  }

  child_env = build_reduced_env(environ, env_names, env_name_count);
  if (child_env == NULL) {
    free_string_array(env_names, env_name_count);
    return EXIT_FAILURE;
  }

  if (build_child_path(child_path, sizeof(child_path)) != 0) {
    free_envp(child_env);
    free_string_array(env_names, env_name_count);
    return EXIT_FAILURE;
  }

  while ((ch = getchar()) != EOF) {
    if (ch == '+' || ch == '*') {
      if (spawn_child(child_path, (char)ch, child_index, child_env) == 0) {
        child_index = (child_index + 1U) % 100U;
      }
    } else if (ch == 'q') {
      break;
    }

    reap_children_nonblocking();
  }

  wait_all_children();
  free_envp(child_env);
  free_string_array(env_names, env_name_count);
  return EXIT_SUCCESS;
}
