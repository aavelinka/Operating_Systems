/*
 * Модуль реализует работу с параметрами запуска и проверку окружения.
 */

#include "options.h"

#include "memory_utils.h"

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_WORKER_COUNT 4U
#define DEFAULT_EXTENSION ".tiff"
#define DEFAULT_WORK_DIR "./work"
#define DEFAULT_PROCESS_SCRIPT "./scripts/process_file.sh"
#define DEFAULT_FINAL_SCRIPT "./scripts/finalize_djvu.sh"

static int parse_worker_count(const char *text_value, size_t *worker_count);
static int has_extension(const char *file_name, const char *extension);
static char *join_path(const char *left, const char *right);

// выводит подсказку по использованию программы.
void print_usage(const char *program_name)
{
    fprintf(
        stderr,
        "Использование:\n"
        "  %s -i <input_dir> -o <output_file> [параметры]\n\n"
        "Параметры:\n"
        "  -i <dir>   Каталог с входными файлами (обязательно).\n"
        "  -o <file>  Имя итогового выходного .djvu файла (обязательно).\n"
        "  -p <path>  Скрипт обработки одного файла (по умолчанию: %s).\n"
        "  -f <path>  Финальный скрипт сборки результата (по умолчанию: %s).\n"
        "  -d <dir>   Каталог для промежуточных .djvu файлов (по умолчанию: %s).\n"
        "  -e <ext>   Расширение входных файлов (по умолчанию: %s).\n"
        "  -w <num>   Число параллельных процессов (>0, по умолчанию: %u).\n"
        "  -h         Показать эту справку.\n",
        program_name,
        DEFAULT_PROCESS_SCRIPT,
        DEFAULT_FINAL_SCRIPT,
        DEFAULT_WORK_DIR,
        DEFAULT_EXTENSION,
        DEFAULT_WORKER_COUNT
    );
}

// инициализирует структуру параметров значениями по умолчанию.
void options_init(options_t *options)
{
    options->input_dir = NULL;
    options->process_script = xstrdup(DEFAULT_PROCESS_SCRIPT);
    options->final_script = xstrdup(DEFAULT_FINAL_SCRIPT);
    options->output_file = NULL;
    options->work_dir = xstrdup(DEFAULT_WORK_DIR);
    options->extension = xstrdup(DEFAULT_EXTENSION);
    options->workers = DEFAULT_WORKER_COUNT;
}

// освобождает динамические ресурсы структуры параметров.
void options_free(options_t *options)
{
    free(options->input_dir);
    options->input_dir = NULL;

    free(options->process_script);
    options->process_script = NULL;

    free(options->final_script);
    options->final_script = NULL;

    free(options->output_file);
    options->output_file = NULL;

    free(options->work_dir);
    options->work_dir = NULL;

    free(options->extension);
    options->extension = NULL;
}

/*
 * Назначение: разбирает аргументы командной строки через getopt.
 * Принимает: argc/argv, структуру параметров и флаг help_requested.
 * Возвращает: 0 при успешном разборе, иначе -1.
 */
int parse_options(int argc, char **argv, options_t *options, int *help_requested)
{
    int option;

    while ((option = getopt(argc, argv, "hi:o:p:f:d:e:w:")) != -1) {
        switch (option) {
            case 'h':
                print_usage(argv[0]);
                *help_requested = 1;
                return -1;
            case 'i':
                free(options->input_dir);
                options->input_dir = xstrdup(optarg);
                break;
            case 'o':
                free(options->output_file);
                options->output_file = xstrdup(optarg);
                break;
            case 'p':
                free(options->process_script);
                options->process_script = xstrdup(optarg);
                break;
            case 'f':
                free(options->final_script);
                options->final_script = xstrdup(optarg);
                break;
            case 'd':
                free(options->work_dir);
                options->work_dir = xstrdup(optarg);
                break;
            case 'e':
                free(options->extension);
                options->extension = xstrdup(optarg);
                break;
            case 'w':
                if (parse_worker_count(optarg, &options->workers) != 0) {
                    fprintf(stderr, "Ошибка: неверное значение для параметра -w: %s\n", optarg);
                    return -1;
                }
                break;
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Ошибка: обнаружены неподдерживаемые позиционные аргументы.\n");
        return -1;
    }

    if (options->input_dir == NULL) {
        fprintf(stderr, "Ошибка: параметр -i обязателен.\n");
        print_usage(argv[0]);
        return -1;
    }

    if (options->output_file == NULL) {
        fprintf(stderr, "Ошибка: параметр -o обязателен.\n");
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

/*
 * Назначение: проверяет, что скрипт существует и имеет право на исполнение.
 * Принимает: путь к скрипту.
 * Возвращает: 0 если проверка пройдена, иначе -1.
 */
int ensure_script_executable(const char *script_path)
{
    if (access(script_path, X_OK) != 0) {
        fprintf(stderr, "Ошибка: скрипт '%s' недоступен для исполнения: %s\n", script_path, strerror(errno));
        return -1;
    }
    return 0;
}

/*
 * Назначение: рекурсивно создает каталог, если он отсутствует.
 * Принимает: путь к каталогу.
 * Возвращает: 0 при успехе, иначе -1.
 */
int ensure_directory(const char *directory_path)
{
    size_t index;
    size_t length;
    char *path_copy;

    if ((directory_path == NULL) || (directory_path[0] == '\0')) {
        fprintf(stderr, "Ошибка: указан пустой путь каталога для промежуточных файлов.\n");
        return -1;
    }

    path_copy = xstrdup(directory_path);
    length = strlen(path_copy);

    for (index = 1U; index <= length; ++index) {
        struct stat path_info;
        char stored;

        if ((path_copy[index] != '/') && (path_copy[index] != '\0')) {
            continue;
        }

        stored = path_copy[index];
        path_copy[index] = '\0';

        if (path_copy[0] != '\0') {
            if (mkdir(path_copy, 0755) != 0) {
                if (errno != EEXIST) {
                    fprintf(stderr, "Ошибка: не удалось создать каталог '%s': %s\n", path_copy, strerror(errno));
                    free(path_copy);
                    return -1;
                }

                if (stat(path_copy, &path_info) != 0) {
                    fprintf(stderr, "Ошибка: не удалось проверить каталог '%s': %s\n", path_copy, strerror(errno));
                    free(path_copy);
                    return -1;
                }

                if (!S_ISDIR(path_info.st_mode)) {
                    fprintf(stderr, "Ошибка: путь '%s' существует, но это не каталог.\n", path_copy);
                    free(path_copy);
                    return -1;
                }
            }
        }

        path_copy[index] = stored;
    }

    free(path_copy);
    return 0;
}

/*
 * Назначение: удаляет промежуточные файлы заданного расширения из каталога.
 * Принимает: путь к каталогу и расширение файлов.
 * Возвращает: 0 при успехе, иначе -1.
 */
int clean_intermediate_files(const char *directory_path, const char *extension)
{
    DIR *directory;
    struct dirent *entry;

    directory = opendir(directory_path);
    if (directory == NULL) {
        fprintf(stderr, "Ошибка: не удалось открыть каталог '%s' для очистки: %s\n", directory_path, strerror(errno));
        return -1;
    }

    for (;;) {
        struct stat file_info;
        char *full_path;

        errno = 0;
        entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                fprintf(stderr, "Ошибка: сбой чтения каталога '%s' при очистке: %s\n", directory_path, strerror(errno));
                if (closedir(directory) != 0) {
                    fprintf(stderr, "Предупреждение: не удалось закрыть каталог '%s': %s\n", directory_path, strerror(errno));
                }
                return -1;
            }
            break;
        }

        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }

        if (has_extension(entry->d_name, extension) == 0) {
            continue;
        }

        full_path = join_path(directory_path, entry->d_name);
        if (lstat(full_path, &file_info) != 0) {
            fprintf(stderr, "Ошибка: не удалось получить сведения о файле '%s': %s\n", full_path, strerror(errno));
            free(full_path);
            if (closedir(directory) != 0) {
                fprintf(stderr, "Предупреждение: не удалось закрыть каталог '%s': %s\n", directory_path, strerror(errno));
            }
            return -1;
        }

        if (S_ISREG(file_info.st_mode)) {
            if (unlink(full_path) != 0) {
                fprintf(stderr, "Ошибка: не удалось удалить файл '%s': %s\n", full_path, strerror(errno));
                free(full_path);
                if (closedir(directory) != 0) {
                    fprintf(stderr, "Предупреждение: не удалось закрыть каталог '%s': %s\n", directory_path, strerror(errno));
                }
                return -1;
            }
        }

        free(full_path);
    }

    if (closedir(directory) != 0) {
        fprintf(stderr, "Ошибка: не удалось закрыть каталог '%s': %s\n", directory_path, strerror(errno));
        return -1;
    }

    return 0;
}

/*
 * Назначение: преобразует текст в количество рабочих процессов.
 * Принимает: строку с числом и указатель для результата.
 * Возвращает: 0 при успехе, иначе -1.
 */
static int parse_worker_count(const char *text_value, size_t *worker_count)
{
    char *end_ptr;
    unsigned long parsed_value;

    errno = 0;
    parsed_value = strtoul(text_value, &end_ptr, 10);
    if ((errno != 0) || (end_ptr == text_value) || (*end_ptr != '\0') || (parsed_value == 0UL)) {
        return -1;
    }

    if ((sizeof(unsigned long) > sizeof(size_t)) && (parsed_value > (unsigned long)SIZE_MAX)) {
        return -1;
    }

    *worker_count = (size_t)parsed_value;
    return 0;
}

/*
 * Назначение: проверяет, что имя файла оканчивается указанным расширением.
 * Принимает: имя файла и расширение.
 * Возвращает: 1 если расширение подходит, иначе 0.
 */
static int has_extension(const char *file_name, const char *extension)
{
    size_t file_length;
    size_t extension_length;

    if ((extension == NULL) || (extension[0] == '\0')) {
        return 1;
    }

    file_length = strlen(file_name);
    extension_length = strlen(extension);
    if (extension_length > file_length) {
        return 0;
    }

    return strcmp(file_name + (file_length - extension_length), extension) == 0;
}

/*
 * Назначение: объединяет путь каталога и имя файла в единый путь.
 * Принимает: левую и правую части пути.
 * Возвращает: новый динамический путь.
 */
static char *join_path(const char *left, const char *right)
{
    size_t left_length;
    size_t right_length;
    int needs_separator;
    size_t total_length;
    char *result;

    left_length = strlen(left);
    right_length = strlen(right);
    needs_separator = (left_length > 0U) && (left[left_length - 1U] != '/');

    total_length = left_length + right_length + (size_t)needs_separator + 1U;
    result = xmalloc(total_length);

    memcpy(result, left, left_length);
    if (needs_separator != 0) {
        result[left_length] = '/';
        memcpy(result + left_length + 1U, right, right_length + 1U);
    } else {
        memcpy(result + left_length, right, right_length + 1U);
    }

    return result;
}
