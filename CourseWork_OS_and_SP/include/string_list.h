#ifndef STRING_LIST_H
#define STRING_LIST_H

#include <stddef.h>

typedef struct string_list_s {
    char **items;
    size_t count;
    size_t capacity;
} string_list_t;

/*
 * Назначение: инициализирует динамический список строк.
 * Принимает: указатель на структуру string_list_t.
 * Возвращает: ничего.
 */
void string_list_init(string_list_t *list);

/*
 * Назначение: добавляет строку в конец динамического списка.
 * Принимает: список и владение строкой value.
 * Возвращает: ничего.
 */
void string_list_push(string_list_t *list, char *value);

/*
 * Назначение: освобождает список строк вместе с его содержимым.
 * Принимает: указатель на структуру string_list_t.
 * Возвращает: ничего.
 */
void string_list_free(string_list_t *list);

/*
 * Назначение: сортирует список строк в лексикографическом порядке.
 * Принимает: указатель на структуру string_list_t.
 * Возвращает: ничего.
 */
void string_list_sort(string_list_t *list);

#endif
