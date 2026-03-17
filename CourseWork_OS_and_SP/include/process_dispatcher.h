#ifndef PROCESS_DISPATCHER_H
#define PROCESS_DISPATCHER_H

#include "options.h"
#include "string_list.h"

/*
 * Назначение: выполняет параллельную диспетчеризацию обработки входных файлов.
 * Принимает: параметры запуска и список входных файлов.
 * Возвращает: 0 при полном успехе, иначе -1.
 */
int run_dispatcher(const options_t *options, const string_list_t *files);

/*
 * Назначение: запускает финальный скрипт сборки результата через execve.
 * Принимает: параметры запуска.
 * Возвращает: 0 при успешном завершении скрипта, иначе -1.
 */
int run_final_script(const options_t *options);

#endif
