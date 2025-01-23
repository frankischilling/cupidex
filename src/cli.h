// File: cli.h
// -----------------------
// Header file for CLI utility functions.

#ifndef CLI_H
#define CLI_H

// Default buffer size for CLI input
#ifndef CLI_LINESZ
#define CLI_LINESZ 256
#endif

/* ─────────────────────────────── Function Declarations ─────────────────────────────── */

/**
 * Reads a line from standard input into a buffer.
 * Returns false in case of an error.
 *
 * @param buf A buffer of at least CLI_LINESZ to store the input.
 * @return true if input is successfully read, false otherwise.
 */
[[nodiscard("false is returned in case of error")]]
bool cli_readline(char[static CLI_LINESZ]);

/**
 * Prints a formatted message to the command line.
 *
 * @param format Format string (similar to printf).
 * @param ... Variadic arguments matching the format specifier.
 */
void cli_println(const char *, ...);

#endif // CLI_H
