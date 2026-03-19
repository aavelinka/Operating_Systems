#ifndef LAB3_TYPES_H
#define LAB3_TYPES_H

#include <stdbool.h>
#include <sys/types.h>

typedef struct {
    int first;
    int second;
} Pair;

typedef struct {
    pid_t pid;
} ChildInfo;

#endif
