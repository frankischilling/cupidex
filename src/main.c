#include <stddef.h>
#include <stdio.h>

#include <cli.h>
#include <utils.h>

int main() {
	cli_println("input something pls");

	char line[CLI_LINESZ];

	if (!cli_readline(line))
		die(1, "Couldn't read line");
	
	

	return 0;
}

