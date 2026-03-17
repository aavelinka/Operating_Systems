/*
 * Модуль реализует диспетчер параллельных дочерних процессов и запуск финальной сборки.
 */

#include "process_dispatcher.h"

#include "memory_utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

#define INTERMEDIATE_EXTENSION ".djvu"

typedef struct worker_slot_s {
    pid_t pid;
    size_t file_index;
    int is_busy;
} worker_slot_t;

static size_t find_free_slot(const worker_slot_t *slots, size_t slot_count);
static size_t find_slot_by_pid(const worker_slot_t *slots, size_t slot_count, pid_t pid);
static int wait_for_child(pid_t *finished_pid, int *status_value);
static int spawn_processing_process(const options_t *options, const char *input_file, pid_t *child_pid);
static int configure_child_environment(const options_t *options);

/*
 * Назначение: выполняет параллельную диспетчеризацию обработки входных файлов.
 * Принимает: параметры запуска и список входных файлов.
 * Возвращает: 0 при полном успехе, иначе -1.
 */
int run_dispatcher(const options_t *options, const string_list_t *files)
{
    worker_slot_t *slots;
    size_t worker_capacity;
    size_t next_file_index;
    size_t active_workers;
    size_t failed_jobs;
    int fatal_error;
    size_t slot_index;

    worker_capacity = options->workers;
    if (worker_capacity > files->count) {
        worker_capacity = files->count;
    }

    if (worker_capacity == 0U) {
        return 0;
    }

    if (worker_capacity > (SIZE_MAX / sizeof(worker_slot_t))) {
        fprintf(stderr, "Ошибка: слишком большое значение числа рабочих процессов.\n");
        return -1;
    }

    slots = xmalloc(worker_capacity * sizeof(worker_slot_t));
    for (slot_index = 0U; slot_index < worker_capacity; ++slot_index) {
        slots[slot_index].pid = (pid_t)0;
        slots[slot_index].file_index = 0U;
        slots[slot_index].is_busy = 0;
    }

    next_file_index = 0U;
    active_workers = 0U;
    failed_jobs = 0U;
    fatal_error = 0;

    while ((next_file_index < files->count) || (active_workers > 0U)) {
        while ((next_file_index < files->count) && (active_workers < worker_capacity)) {
            pid_t child_pid;

            slot_index = find_free_slot(slots, worker_capacity);
            if (slot_index == worker_capacity) {
                fatal_error = 1;
                break;
            }

            if (spawn_processing_process(options, files->items[next_file_index], &child_pid) != 0) {
                fatal_error = 1;
                next_file_index = files->count;
                break;
            }

            slots[slot_index].pid = child_pid;
            slots[slot_index].file_index = next_file_index;
            slots[slot_index].is_busy = 1;
            active_workers += 1U;

            fprintf(
                stdout,
                "[dispatch] pid=%ld <- %s\n",
                (long)child_pid,
                files->items[next_file_index]
            );
            fflush(stdout);

            next_file_index += 1U;
        }

        if (active_workers == 0U) {
            break;
        }

        {
            pid_t finished_pid;
            int child_status;
            size_t finished_slot;

            if (wait_for_child(&finished_pid, &child_status) != 0) {
                fatal_error = 1;
                break;
            }

            finished_slot = find_slot_by_pid(slots, worker_capacity, finished_pid);
            if (finished_slot == worker_capacity) {
                fprintf(stderr, "Ошибка: завершился неизвестный дочерний процесс pid=%ld.\n", (long)finished_pid);
                fatal_error = 1;
                break;
            }

            slots[finished_slot].is_busy = 0;
            slots[finished_slot].pid = (pid_t)0;
            active_workers -= 1U;

            if (WIFEXITED(child_status) && (WEXITSTATUS(child_status) == 0)) {
                fprintf(
                    stdout,
                    "[done] pid=%ld -> %s\n",
                    (long)finished_pid,
                    files->items[slots[finished_slot].file_index]
                );
            } else if (WIFEXITED(child_status)) {
                fprintf(
                    stderr,
                    "Ошибка: обработка файла '%s' завершилась с кодом %d (pid=%ld).\n",
                    files->items[slots[finished_slot].file_index],
                    WEXITSTATUS(child_status),
                    (long)finished_pid
                );
                failed_jobs += 1U;
            } else if (WIFSIGNALED(child_status)) {
                fprintf(
                    stderr,
                    "Ошибка: обработка файла '%s' прервана сигналом %d (pid=%ld).\n",
                    files->items[slots[finished_slot].file_index],
                    WTERMSIG(child_status),
                    (long)finished_pid
                );
                failed_jobs += 1U;
            } else {
                fprintf(
                    stderr,
                    "Ошибка: непредусмотренный статус дочернего процесса для файла '%s' (pid=%ld).\n",
                    files->items[slots[finished_slot].file_index],
                    (long)finished_pid
                );
                failed_jobs += 1U;
            }
        }
    }

    while (active_workers > 0U) {
        pid_t finished_pid;
        int child_status;
        size_t finished_slot;

        if (wait_for_child(&finished_pid, &child_status) != 0) {
            fatal_error = 1;
            break;
        }

        finished_slot = find_slot_by_pid(slots, worker_capacity, finished_pid);
        if (finished_slot != worker_capacity) {
            slots[finished_slot].is_busy = 0;
            slots[finished_slot].pid = (pid_t)0;
        }

        active_workers -= 1U;
    }

    free(slots);

    if (fatal_error != 0) {
        fprintf(stderr, "Ошибка: диспетчер завершился аварийно.\n");
        return -1;
    }

    if (failed_jobs != 0U) {
        fprintf(stderr, "Ошибка: количество файлов с неуспешной обработкой: %lu.\n", (unsigned long)failed_jobs);
        return -1;
    }

    return 0;
}

/*
 * Назначение: запускает финальный скрипт сборки результата через execve.
 * Принимает: параметры запуска.
 * Возвращает: 0 при успешном завершении скрипта, иначе -1.
 */
int run_final_script(const options_t *options)
{
    pid_t pid;
    pid_t wait_result;
    int status_value;

    fprintf(stdout, "[final] запуск скрипта сборки результата...\n");
    fflush(stdout);

    pid = fork();
    if (pid < (pid_t)0) {
        fprintf(stderr, "Ошибка: fork() для финального скрипта завершился ошибкой: %s\n", strerror(errno));
        return -1;
    }

    if (pid == (pid_t)0) {
        char *const child_argv[] = {
            (char *)options->final_script,
            (char *)"-o",
            (char *)options->output_file,
            NULL
        };
        if (configure_child_environment(options) != 0) {
            _exit(127);
        }
        execve(options->final_script, child_argv, environ);

        fprintf(stderr, "Ошибка: execve('%s') завершился ошибкой: %s\n", options->final_script, strerror(errno));
        _exit(127);
    }

    do {
        wait_result = waitpid(pid, &status_value, 0);
    } while ((wait_result == (pid_t)-1) && (errno == EINTR));

    if (wait_result == (pid_t)-1) {
        fprintf(stderr, "Ошибка: waitpid() для финального скрипта завершился ошибкой: %s\n", strerror(errno));
        return -1;
    }

    if (WIFEXITED(status_value) && (WEXITSTATUS(status_value) == 0)) {
        fprintf(stdout, "[final] итоговый файл сформирован: %s\n", options->output_file);
        return 0;
    }

    if (WIFEXITED(status_value)) {
        fprintf(stderr, "Ошибка: финальный скрипт завершился с кодом %d.\n", WEXITSTATUS(status_value));
    } else if (WIFSIGNALED(status_value)) {
        fprintf(stderr, "Ошибка: финальный скрипт прерван сигналом %d.\n", WTERMSIG(status_value));
    } else {
        fprintf(stderr, "Ошибка: финальный скрипт завершился с непредусмотренным статусом.\n");
    }

    return -1;
}

/*
 * Назначение: ищет свободный слот рабочего процесса.
 * Принимает: массив слотов и его размер.
 * Возвращает: индекс свободного слота или slot_count, если свободного нет.
 */
static size_t find_free_slot(const worker_slot_t *slots, size_t slot_count)
{
    size_t index;

    for (index = 0U; index < slot_count; ++index) {
        if (slots[index].is_busy == 0) {
            return index;
        }
    }

    return slot_count;
}

/*
 * Назначение: находит слот по pid дочернего процесса.
 * Принимает: массив слотов, его размер и pid процесса.
 * Возвращает: индекс найденного слота или slot_count.
 */
static size_t find_slot_by_pid(const worker_slot_t *slots, size_t slot_count, pid_t pid)
{
    size_t index;

    for (index = 0U; index < slot_count; ++index) {
        if ((slots[index].is_busy != 0) && (slots[index].pid == pid)) {
            return index;
        }
    }

    return slot_count;
}

/*
 * Назначение: ожидает завершение любого дочернего процесса.
 * Принимает: указатели для pid завершившегося процесса и его статуса.
 * Возвращает: 0 при успехе, иначе -1.
 */
static int wait_for_child(pid_t *finished_pid, int *status_value)
{
    pid_t wait_result;

    do {
        wait_result = waitpid((pid_t)-1, status_value, 0);
    } while ((wait_result == (pid_t)-1) && (errno == EINTR));

    if (wait_result == (pid_t)-1) {
        fprintf(stderr, "Ошибка: waitpid завершился с ошибкой: %s\n", strerror(errno));
        return -1;
    }

    *finished_pid = wait_result;
    return 0;
}

/*
 * Назначение: запускает скрипт обработки одного файла через fork + execve.
 * Принимает: параметры, путь к файлу и указатель для pid.
 * Возвращает: 0 при успехе, иначе -1.
 */
static int spawn_processing_process(const options_t *options, const char *input_file, pid_t *child_pid)
{
    pid_t pid;

    pid = fork();
    if (pid < (pid_t)0) {
        fprintf(stderr, "Ошибка: fork() для файла '%s' завершился ошибкой: %s\n", input_file, strerror(errno));
        return -1;
    }

    if (pid == (pid_t)0) {
        char *const child_argv[] = {
            (char *)options->process_script,
            (char *)input_file,
            NULL
        };
        if (configure_child_environment(options) != 0) {
            _exit(127);
        }
        execve(options->process_script, child_argv, environ);

        fprintf(stderr, "Ошибка: execve('%s') завершился ошибкой: %s\n", options->process_script, strerror(errno));
        _exit(127);
    }

    *child_pid = pid;
    return 0;
}

/*
 * Назначение: дополняет окружение дочернего процесса рабочими переменными.
 * Принимает: параметры запуска.
 * Возвращает: 0 при успехе, иначе -1.
 */
static int configure_child_environment(const options_t *options)
{
    if (setenv("PROCESS_OUTPUT_DIR", options->work_dir, 1) != 0) {
        fprintf(stderr, "Ошибка: setenv(PROCESS_OUTPUT_DIR) завершился ошибкой: %s\n", strerror(errno));
        return -1;
    }

    if (setenv("PROCESS_OUTPUT_EXTENSION", INTERMEDIATE_EXTENSION, 1) != 0) {
        fprintf(stderr, "Ошибка: setenv(PROCESS_OUTPUT_EXTENSION) завершился ошибкой: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
