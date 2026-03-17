//Модуль реализует динамический список строк.
#include "string_list.h"

#include "memory_utils.h"

#include <stdlib.h>
#include <string.h>

static int compare_strings(const void *left, const void *right);

// инициализирует динамический список строк.
void string_list_init(string_list_t *list)
{
    list->items = NULL;
    list->count = 0U;
    list->capacity = 0U;
}

// добавляет строку в конец динамического списка.
void string_list_push(string_list_t *list, char *value)
{
    if (list->count == list->capacity) {
        size_t new_capacity;
        char **new_items;

        new_capacity = (list->capacity == 0U) ? 8U : (list->capacity * 2U);
        new_items = xrealloc(list->items, new_capacity * sizeof(char *));
        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count] = value;
    list->count += 1U;
}

// освобождает список строк вместе с его содержимым.
void string_list_free(string_list_t *list)
{
    size_t index;

    for (index = 0U; index < list->count; ++index) {
        free(list->items[index]);
        list->items[index] = NULL;
    }

    free(list->items);
    list->items = NULL;
    list->count = 0U;
    list->capacity = 0U;
}


// сортирует список строк в лексикографическом порядке.
void string_list_sort(string_list_t *list)
{
    if (list->count > 1U) {
        qsort(list->items, list->count, sizeof(char *), compare_strings);
    }
}

// компаратор строк для qsort
static int compare_strings(const void *left, const void *right)
{
    const char *const *left_text;
    const char *const *right_text;

    left_text = (const char *const *)left;
    right_text = (const char *const *)right;
    return strcmp(*left_text, *right_text);
}
