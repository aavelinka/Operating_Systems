#include "strlist.h"

#include <stdlib.h>
#include <string.h>

static int compare_locale(const void *lhs, const void *rhs)
{
    const char *a = *(const char *const *)lhs;
    const char *b = *(const char *const *)rhs;
    return strcoll(a, b);
}

void strlist_init(StrList *list)
{
    list->items = NULL;
    list->size = 0;
    list->capacity = 0;
}

void strlist_free(StrList *list)
{
    size_t i;
    for (i = 0; i < list->size; ++i) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->size = 0;
    list->capacity = 0;
}

int strlist_push(StrList *list, const char *value)
{
    char **new_items;
    char *copy;

    if (list->size == list->capacity) {
        size_t new_capacity = list->capacity ? list->capacity * 2 : 64;
        new_items = realloc(list->items, new_capacity * sizeof(*new_items));
        if (!new_items) {
            return -1;
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }

    copy = malloc(strlen(value) + 1);
    if (!copy) {
        return -1;
    }
    strcpy(copy, value);
    list->items[list->size++] = copy;
    return 0;
}

void strlist_sort_locale(StrList *list)
{
    if (list->size > 1) {
        qsort(list->items, list->size, sizeof(*list->items), compare_locale);
    }
}
