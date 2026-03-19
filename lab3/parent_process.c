#define _POSIX_C_SOURCE 200809L

#include "parent_process.h"

#include "child_process.h"
#include "lab3_config.h"
#include "lab3_types.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

typedef struct {
    bool enabled;
    struct termios saved;
} TerminalMode;

static size_t find_child(ChildInfo *children, size_t count, pid_t pid) {
    for (size_t i = 0; i < count; ++i) {
        if (children[i].pid == pid) {
            return i;
        }
    }
    return count;
}

static void remove_child_at(ChildInfo *children, size_t *count, size_t idx) {
    for (size_t i = idx; i + 1 < *count; ++i) {
        children[i] = children[i + 1];
    }
    *count -= 1;
}

static void terminate_one_child(pid_t pid) {
    if (kill(pid, SIGUSR1) == -1 && errno != ESRCH) {
        fprintf(stderr, "[PARENT pid=%ld] failed SIGUSR1 child pid=%ld: %s\n", (long)getpid(),
                (long)pid, strerror(errno));
    }

    int status = 0;
    while (waitpid(pid, &status, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }
        if (errno != ECHILD) {
            fprintf(stderr, "[PARENT pid=%ld] waitpid(%ld) failed: %s\n", (long)getpid(),
                    (long)pid, strerror(errno));
        }
        break;
    }
}

static void kill_all_children(ChildInfo *children, size_t *count) {
    while (*count > 0) {
        pid_t pid = children[*count - 1].pid;
        *count -= 1;
        terminate_one_child(pid);
    }
}

static void reap_children(ChildInfo *children, size_t *count) {
    while (true) {
        int status = 0;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0) {
            break;
        }

        size_t idx = find_child(children, *count, pid);
        if (idx == *count) {
            continue;
        }

        remove_child_at(children, count, idx);

        printf("[PARENT pid=%ld] child pid=%ld finished, remaining=%zu\n", (long)getpid(),
               (long)pid, *count);
    }
}

static void print_processes(const ChildInfo *children, size_t count) {
    printf("[PARENT pid=%ld] children (%zu):\n", (long)getpid(), count);
    for (size_t i = 0; i < count; ++i) {
        printf("[PARENT pid=%ld]   #%zu child pid=%ld\n", (long)getpid(), i,
               (long)children[i].pid);
    }
}

// переводит stdin в неканонический режим
static int enable_immediate_input(TerminalMode *mode) {
    memset(mode, 0, sizeof(*mode));

    if (!isatty(STDIN_FILENO)) {
        return 0;
    }

    if (tcgetattr(STDIN_FILENO, &mode->saved) == -1) {
        perror("tcgetattr");
        return -1;
    }

    // ICANON выключается, поэтому ввод больше не ждет Enter
    // ECHO выключается, поэтому терминал сам не печатает символ
    struct termios raw = mode->saved;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    // чтение возвращает сразу 1 символ:
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        return -1;
    }

    mode->enabled = true;
    return 0;
}

static void restore_terminal_mode(const TerminalMode *mode) {
    if (!mode->enabled) {
        return;
    }

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &mode->saved) == -1) {
        perror("tcsetattr");
    }
}

static int read_command_char(char *cmd) {
    while (true) {
        ssize_t read_count = read(STDIN_FILENO, cmd, 1);
        if (read_count == 1) {
            return 1;
        }
        if (read_count == 0) {
            return 0;
        }
        if (errno == EINTR) {
            continue;
        }

        perror("read");
        return -1;
    }
}

int parent_run(void) {
    ChildInfo *children = calloc((size_t)INITIAL_CAPACITY, sizeof(*children));
    if (children == NULL) {
        perror("calloc children");
        return EXIT_FAILURE;
    }

    size_t children_cap = INITIAL_CAPACITY;
    size_t children_count = 0;
    TerminalMode terminal_mode;

    if (enable_immediate_input(&terminal_mode) == -1) {
        free(children);
        return EXIT_FAILURE;
    }

    printf("[PARENT pid=%ld] commands:\n", (long)getpid());
    printf("  +  create child\n");
    printf("  -  remove last child\n");
    printf("  l  list all processes\n");
    printf("  k  kill all children\n");
    printf("  q  quit\n");

    bool running = true;

    while (running) {
        reap_children(children, &children_count);

        printf("cmd> ");
        fflush(stdout);

        char cmd = '\0';
        int read_status = read_command_char(&cmd);
        if (read_status <= 0) {
            printf("\n");
            break;
        }

        if (terminal_mode.enabled) {
            if (cmd != '\n' && cmd != '\r') {
                putchar(cmd);
            }
            printf("\n");
        }

        if (cmd == '+') {
            if (children_count == children_cap) {
                size_t new_cap = children_cap * 2;
                ChildInfo *new_children = realloc(children, new_cap * sizeof(*new_children));
                if (new_children == NULL) {
                    perror("realloc children");
                    continue;
                }
                children = new_children;
                children_cap = new_cap;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                continue;
            }
            if (pid == 0) {
                child_main(true);
            }

            children[children_count].pid = pid;
            children_count += 1;

            printf("[PARENT pid=%ld] created child pid=%ld, total=%zu\n", (long)getpid(),
                   (long)pid, children_count);
        } else if (cmd == '-') {
            if (children_count == 0) {
                printf("[PARENT pid=%ld] no children to remove\n", (long)getpid());
                continue;
            }

            pid_t pid = children[children_count - 1].pid;
            children_count -= 1;
            terminate_one_child(pid);

            printf("[PARENT pid=%ld] removed child pid=%ld, remaining=%zu\n", (long)getpid(),
                   (long)pid, children_count);
        } else if (cmd == 'l' || cmd == 'L') {
            print_processes(children, children_count);
        } else if (cmd == 'k' || cmd == 'K') {
            kill_all_children(children, &children_count);
            printf("[PARENT pid=%ld] all children removed\n", (long)getpid());
        } else if (cmd == 'q' || cmd == 'Q') {
            kill_all_children(children, &children_count);
            running = false;
            printf("[PARENT pid=%ld] quit\n", (long)getpid());
        } else if (cmd == '\n') {
            continue;
        } else {
            printf("[PARENT pid=%ld] unknown command: %c\n", (long)getpid(), cmd);
        }
    }

    if (children_count > 0) {
        kill_all_children(children, &children_count);
    }

    restore_terminal_mode(&terminal_mode);
    free(children);
    return EXIT_SUCCESS;
}
