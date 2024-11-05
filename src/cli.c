// File: cil.c
// -----------------------
#include <stdarg.h>   // for va_list, va_start, va_end
#include <stdbool.h>  // for bool, true, false
#include <stddef.h>   // for NULL
#include <stdio.h>    // for printf, vprintf, fgets, stdin
#include <string.h>   // for strcspn

#include "cli.h"

bool cli_readline(char buf[static CLI_LINESZ]) {
    if (NULL == fgets(buf, CLI_LINESZ, stdin))
        return false;
    buf[strcspn(buf, "\n")] = '\0';
    return true;
}

void cli_println(const char *format, ...) {
    printf(">");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}

