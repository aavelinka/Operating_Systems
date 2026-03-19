#define _POSIX_C_SOURCE 200809L

#include "child_process.h"

#include "lab3_config.h"
#include "lab3_types.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_child_alarm = 0;
static volatile sig_atomic_t g_child_stop = 0;
static volatile sig_atomic_t g_child_print_enabled = 1;

static void child_signal_handler(int sig) {
    if (sig == SIGALRM) {
        g_child_alarm = 1;
    } else if (sig == SIGUSR1) {
        g_child_stop = 1;
    } else if (sig == SIGUSR2) {
        g_child_print_enabled = !g_child_print_enabled;
    }
}

static void child_set_timer(useconds_t usec) {
    struct itimerval timer;
    memset(&timer, 0, sizeof(timer));
    timer.it_value.tv_sec = (time_t)(usec / 1000000U);
    timer.it_value.tv_usec = (suseconds_t)(usec % 1000000U);
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        _exit(EXIT_FAILURE);
    }
}

static void nanosleep_gap(long nsec) {
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = nsec;
    (void)nanosleep(&req, NULL);
}

void child_main(bool output_enabled) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = child_signal_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1 ||
        sigaction(SIGUSR1, &sa, NULL) == -1 ||
        sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction(child)");
        _exit(EXIT_FAILURE);
    }

    g_child_stop = 0;
    g_child_alarm = 0;
    g_child_print_enabled = output_enabled ? 1 : 0;

    Pair pair = {0, 0};
    unsigned long stats[2][2] = {{0, 0}, {0, 0}};
    int samples = 0;

    printf("[CHILD pid=%ld ppid=%ld] started, output=%s\n", (long)getpid(), (long)getppid(),
           g_child_print_enabled ? "ON" : "OFF");
    fflush(stdout);

    while (!g_child_stop) {
        g_child_alarm = 0;
        child_set_timer((useconds_t)TIMER_USEC);

        while (!g_child_alarm && !g_child_stop) {
            pair.first = 0;
            nanosleep_gap((long)WRITE_GAP_NSEC);
            if (g_child_alarm || g_child_stop) {
                break;
            }

            pair.second = 0;
            nanosleep_gap((long)WRITE_GAP_NSEC);
            if (g_child_alarm || g_child_stop) {
                break;
            }

            pair.first = 1;
            nanosleep_gap((long)WRITE_GAP_NSEC);
            if (g_child_alarm || g_child_stop) {
                break;
            }

            pair.second = 1;
            nanosleep_gap((long)WRITE_GAP_NSEC);
        }

        if (g_child_stop) {
            break;
        }

        stats[pair.first][pair.second] += 1UL;
        samples += 1;

        if (samples >= REPORT_EVERY) {
            if (g_child_print_enabled) {
                printf("[CHILD pid=%ld ppid=%ld] 00=%lu 01=%lu 10=%lu 11=%lu\n", (long)getpid(),
                       (long)getppid(), stats[0][0], stats[0][1], stats[1][0], stats[1][1]);
                fflush(stdout);
            }
            memset(stats, 0, sizeof(stats));
            samples = 0;
        }
    }

    printf("[CHILD pid=%ld ppid=%ld] stopped\n", (long)getpid(), (long)getppid());
    fflush(stdout);
    _exit(EXIT_SUCCESS);
}
