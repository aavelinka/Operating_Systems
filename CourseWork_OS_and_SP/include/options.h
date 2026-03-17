#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>

typedef struct options_s {
    char *input_dir;
    char *process_script;
    char *final_script;
    char *output_file;
    char *work_dir;
    char *extension;
    size_t workers;
} options_t;

/*
 * Назначение: выводит подсказку по использованию программы.
 * Принимает: имя исполняемого файла.
 * Возвращает: ничего.
 */
void print_usage(const char *program_name);

/*
 * Назначение: инициализирует структуру параметров значениями по умолчанию.
 * Принимает: указатель на структуру options_t.
 * Возвращает: ничего.
 */
void options_init(options_t *options);

/*
 * Назначение: освобождает динамические ресурсы структуры параметров.
 * Принимает: указатель на структуру options_t.
 * Возвращает: ничего.
 */
void options_free(options_t *options);

/*
 * Назначение: разбирает аргументы командной строки через getopt.
 * Принимает: argc/argv, структуру параметров и флаг help_requested.
 * Возвращает: 0 при успешном разборе, иначе -1.
 */
int parse_options(int argc, char **argv, options_t *options, int *help_requested);

/*
 * Назначение: проверяет, что скрипт существует и имеет право на исполнение.
 * Принимает: путь к скрипту.
 * Возвращает: 0 если проверка пройдена, иначе -1.
 */
int ensure_script_executable(const char *script_path);

/*
 * Назначение: рекурсивно создает каталог, если он отсутствует.
 * Принимает: путь к каталогу.
 * Возвращает: 0 при успехе, иначе -1.
 */
int ensure_directory(const char *directory_path);

/*
 * Назначение: удаляет промежуточные файлы заданного расширения из каталога.
 * Принимает: путь к каталогу и расширение файлов.
 * Возвращает: 0 при успехе, иначе -1.
 */
int clean_intermediate_files(const char *directory_path, const char *extension);

#endif
