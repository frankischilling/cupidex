#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>


#include "utils.h"

void die(int r, const char *format, ...) {
	fprintf(stderr, "The program used die()\n");
	fprintf(stderr, "The last errno was %d/%s\n", errno, strerror(errno));
	fprintf(stderr, "The user of die() decided to leave this message for "
			"you:\n");

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, "\nGood Luck.\n");

	exit(r);
}


void create_file(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f == NULL)
        die(1, "Couldn't create file %s", filename);
    fclose(f);
}


//TODO replace edit_file by calling the system editor
void edit_file(const char *filename) {
    const char *editor = getenv("EDITOR");

    // Use the default text editor from the environment variable if set
    if (editor != NULL && editor[0] != '\0') {
        // Use the system default text editor to open the file
        char command[256];
        snprintf(command, sizeof(command), "%s %s", editor, filename);

        int result = system(command);

        if (result == -1) {
            // Error launching the editor
            printf("Error: Unable to open the editor specified in the EDITOR environment variable.\n");
        } else if (WIFEXITED(result) && WEXITSTATUS(result) != 0) {
            // The editor exited with a non-zero status, indicating an issue
            printf("Error: The editor specified in the EDITOR environment variable returned a non-zero status.\n");
        }
    } else {
        // Use the default editor specified by a macro as a last resort
        char command[256];
        snprintf(command, sizeof(command), "%s %s", EDITOR_COMMAND, filename);

        int result = system(command);

        if (result == -1) {
            // Error launching the default editor
            printf("Error: Unable to open the default editor.\n");
        } else if (WIFEXITED(result) && WEXITSTATUS(result) != 0) {
            // The default editor exited with a non-zero status, indicating an issue
            printf("Error: The default editor returned a non-zero status.\n");
        }
    }
}
