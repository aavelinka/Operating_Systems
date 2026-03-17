/*
 * Модуль содержит точку входа и высокоуровневый сценарий работы программы.
 */

#include "file_collector.h"
#include "options.h"
#include "process_dispatcher.h"
#include "string_list.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Назначение: точка входа программы диспетчера.
 * Принимает: argc/argv с параметрами командной строки.
 * Возвращает: EXIT_SUCCESS при успехе, иначе EXIT_FAILURE.
 */
int main(int argc, char **argv)
{
    options_t options;
    string_list_t files;
    int status_code;
    int help_requested;

    if (setlocale(LC_ALL, "") == NULL) {
        fprintf(stderr, "Предупреждение: не удалось установить локаль, продолжаем в системной локали.\n");
    }

    options_init(&options);
    string_list_init(&files);
    status_code = EXIT_FAILURE;
    help_requested = 0;

    if (argc == 1) {
        print_usage(argv[0]);
        goto cleanup;
    }

    if (parse_options(argc, argv, &options, &help_requested) != 0) {
        status_code = (help_requested != 0) ? EXIT_SUCCESS : EXIT_FAILURE;
        goto cleanup;
    }

    if (ensure_script_executable(options.process_script) != 0) {
        goto cleanup;
    }

    if (ensure_script_executable(options.final_script) != 0) {
        goto cleanup;
    }

    if (ensure_directory(options.work_dir) != 0) {
        goto cleanup;
    }

    if (clean_intermediate_files(options.work_dir, ".djvu") != 0) {
        goto cleanup;
    }

    if (collect_input_files(options.input_dir, options.extension, &files) != 0) {
        goto cleanup;
    }

    if (files.count == 0U) {
        fprintf(stderr, "Ошибка: во входном каталоге не найдено файлов с расширением '%s'.\n", options.extension);
        goto cleanup;
    }

    if (run_dispatcher(&options, &files) != 0) {
        goto cleanup;
    }

    if (run_final_script(&options) != 0) {
        goto cleanup;
    }

    status_code = EXIT_SUCCESS;

cleanup:
    string_list_free(&files);
    options_free(&options);
    return status_code;
}
