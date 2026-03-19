#define _POSIX_C_SOURCE 200809L

#include "parent_process.h"

#include <stdio.h>

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    return parent_run();
}
