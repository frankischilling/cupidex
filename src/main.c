#include <stddef.h>
#include <stdio.h>

#include <cli.h>
#include <utils.h>

int main() {
    cli_println("Welcome to cupidfm - a console-based file manager");
    cli_println("input something pls");

	char line[CLI_LINESZ];
    char filename[CLI_LINESZ]; // Declare the filename variable

    if (!cli_readline(line))
		die(1, "Couldn't read line");

    cli_println("Enter the filename you want to create:");
    if (!cli_readline(filename)) {
        die(1, "Couldn't read filename");
    }


    // create
    create_file(filename);
    cli_println("File created: %s", filename);

    // edit
    edit_file(filename);
    cli_println("File edited: %s", filename);

	return 0;
}

