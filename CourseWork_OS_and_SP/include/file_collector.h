#ifndef FILE_COLLECTOR_H
#define FILE_COLLECTOR_H

#include "string_list.h"

/*
 * Назначение: собирает список входных файлов из каталога по расширению.
 * Принимает: путь каталога, расширение и список-накопитель.
 * Возвращает: 0 при успехе, иначе -1.
 */
int collect_input_files(const char *input_dir, const char *extension, string_list_t *files);

#endif
