// File: cil.c
// -----------------------
// Implements basic command-line interface (CLI) utilities.

#include <stdarg.h>   // va_list, va_start, va_end
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // printf, vprintf, fgets, stdin
#include <string.h>   // strcspn

// Local includes
#include "cli.h"

/**
 * Reads a line from the standard input into the provided buffer.
 * Trims the newline character if present.
 *
 * @param buf A buffer of at least CLI_LINESZ to store the input.
 * @return true if input was successfully read, false otherwise.
 */
bool cli_readline(char buf[static CLI_LINESZ]) {
    if (fgets(buf, CLI_LINESZ, stdin) == NULL)
        return false;
    
    // Trim newline character if present
    buf[strcspn(buf, "\n")] = '\0';
    return true;
}

/**
 * Prints a formatted line to the command line, prefixed with '>'.
 *
 * @param format Format string (similar to printf).
 * @param ... Variadic arguments matching the format specifier.
 */
void cli_println(const char *format, ...) {
    printf(">");  // CLI prompt prefix

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}
