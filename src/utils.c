#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    FILE *f = fopen(filename, "r");
    if (f != NULL) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), f) != NULL) {
            printf("%s", buffer);
        }

        // Close the file
        fclose(f);

        // Open the file again for writing
        f = fopen(filename, "a");
        if (f != NULL) {
            // Allow user to add text to the file
            printf("\nEnter new content (Ctrl+D to save and exit):\n");
            while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                fprintf(f, "%s", buffer);
            }

            // Close the file
            fclose(f);
        } else {
            printf("Error: Unable to open file.\n");
        }

    }
}
