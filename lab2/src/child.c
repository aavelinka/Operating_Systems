#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

extern char **environ;

static void print_env(char *const envp[]) {
  size_t i = 0;

  while (envp != NULL && envp[i] != NULL) {
    puts(envp[i]);
    i++;
  }
}

int main(int argc, char *argv[], char *envp[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <mode>\n", argv[0]);
    return EXIT_FAILURE;
  }

  printf("%s pid=%ld ppid=%ld\n", argv[0], (long)getpid(), (long)getppid());

  if (strcmp(argv[1], "+") == 0) {
    print_env(environ);
  } else if (strcmp(argv[1], "*") == 0) {
    print_env(envp);
  } else {
    fprintf(stderr, "unknown mode: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
