#ifndef STRLIST_H
#define STRLIST_H

#include <stddef.h>

typedef struct {
    char **items;
    size_t size;
    size_t capacity;
} StrList;

void strlist_init(StrList *list);
void strlist_free(StrList *list);
int strlist_push(StrList *list, const char *value);
void strlist_sort_locale(StrList *list);

#endif
