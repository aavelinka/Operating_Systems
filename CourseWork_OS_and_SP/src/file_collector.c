/*
 * Модуль реализует сбор входных файлов из каталога.
 */

#include "file_collector.h"

#include "memory_utils.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *join_path(const char *left, const char *right);
static int has_extension(const char *file_name, const char *extension);

/*
 * Назначение: собирает список входных файлов из каталога по расширению.
 * Принимает: путь каталога, расширение и список-накопитель.
 * Возвращает: 0 при успехе, иначе -1.
 */
int collect_input_files(const char *input_dir, const char *extension, string_list_t *files)
{
    DIR *directory;
    struct dirent *entry;

    directory = opendir(input_dir);
    if (directory == NULL) {
        fprintf(stderr, "Ошибка: не удалось открыть каталог '%s': %s\n", input_dir, strerror(errno));
        return -1;
    }

    for (;;) {
        struct stat file_info;
        char *full_path;

        errno = 0;
        entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                fprintf(stderr, "Ошибка: сбой чтения каталога '%s': %s\n", input_dir, strerror(errno));
                if (closedir(directory) != 0) {
                    fprintf(stderr, "Предупреждение: не удалось закрыть каталог '%s': %s\n", input_dir, strerror(errno));
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

        full_path = join_path(input_dir, entry->d_name);
        if (lstat(full_path, &file_info) != 0) {
            fprintf(stderr, "Ошибка: не удалось получить сведения о файле '%s': %s\n", full_path, strerror(errno));
            free(full_path);
            continue;
        }

        if (S_ISREG(file_info.st_mode)) {
            string_list_push(files, full_path);
        } else {
            free(full_path);
        }
    }

    if (closedir(directory) != 0) {
        fprintf(stderr, "Ошибка: не удалось закрыть каталог '%s': %s\n", input_dir, strerror(errno));
        return -1;
    }

    string_list_sort(files);
    return 0;
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
