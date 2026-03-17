/*
 * Модуль содержит функции безопасной работы с динамической памятью.
 */

#include "memory_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Назначение: выделяет память и аварийно завершает процесс при неудаче.
 * Принимает: размер выделяемого блока.
 * Возвращает: указатель на выделенную память.
 */
void *xmalloc(size_t size)
{
    void *memory;

    memory = malloc(size);
    if (memory == NULL) {
        fprintf(stderr, "Критическая ошибка: malloc(%lu) вернул NULL.\n", (unsigned long)size);
        abort();
    }

    return memory;
}

/*
 * Назначение: перераспределяет память и аварийно завершает процесс при неудаче.
 * Принимает: исходный указатель и новый размер.
 * Возвращает: указатель на новый блок памяти.
 */
void *xrealloc(void *memory, size_t size)
{
    void *new_memory;

    new_memory = realloc(memory, size);
    if (new_memory == NULL) {
        fprintf(stderr, "Критическая ошибка: realloc(..., %lu) вернул NULL.\n", (unsigned long)size);
        abort();
    }

    return new_memory;
}

/*
 * Назначение: дублирует строку в динамической памяти.
 * Принимает: исходную C-строку.
 * Возвращает: новый дублированный буфер.
 */
char *xstrdup(const char *source)
{
    size_t length;
    char *result;

    length = strlen(source);
    result = xmalloc(length + 1U);
    memcpy(result, source, length + 1U);
    return result;
}
