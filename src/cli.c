#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"

bool cli_readline(char buf[static CLI_LINESZ]) {
	if (nullptr == fgets(buf, CLI_LINESZ, stdin))
		return false;
	buf[strcspn(buf, "\n")] = '\0';
	return true;
}

bool cli_println(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);

	printf("\n");
}

