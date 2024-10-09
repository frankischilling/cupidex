// File: utils.c
// -----------------------
#include <errno.h>     // for errno
#include <stdarg.h>    // for va_list, va_start, va_end
#include <stdio.h>     // for fprintf, stderr, vfprintf
#include <stdlib.h>    // for exit
#include <string.h>    // for strerror
#include <sys/wait.h>  // for WEXITSTATUS, WIFEXITED
#include <dirent.h>    // for DIR, struct dirent, opendir, readdir, closedir
#include <curses.h>    // for initscr, noecho, keypad, stdscr, clear, printw, refresh, mvaddch, getch, endwin
#include <unistd.h>    // for system
#include <sys/types.h> // for stat
#include <sys/stat.h>  // for stat, S_ISDIR
// Local includes
#include "utils.h"

#define MAX_PATH_LENGTH 256
#define MAX_DISPLAY_LENGTH 32


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

void browse_files(const char *directory) {
    char command[256];
    snprintf(command, sizeof(command), "xdg-open %s", directory);

    int result = system(command);

    if (result == -1) {
        // Error launching the file manager
        printf("Error: Unable to open the file manager.\n");
    } else if (WIFEXITED(result) && WEXITSTATUS(result) != 0) {
        // The file manager exited with a non-zero status, indicating an issue
        printf("Error: The file manager returned a non-zero status.\n");
    }
}

void display_files(const char *directory) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(directory)) == NULL) {
        die(1, "Couldn't open directory %s", directory);
        return ;
    }

    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
}

void preview_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Couldn't open file %s for preview\n", filename);
        return;
    }

    initscr();
    noecho();
    keypad(stdscr, TRUE);

    int ch;
    int row = 0;
    int col = 0;

    clear();
    printw("Previewing file: %s\n", filename);
    refresh();

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            row++;
            col = 0;
        } else {
            mvaddch(row, col, ch);
            col++;
        }
    }

    refresh();

    getch();  // Wait for user input before closing preview

    endwin();
    fclose(file);
}

bool is_directory(const char *path, const char *filename) {
    struct stat path_stat;
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    if (stat(full_path, &path_stat) == 0)
        return S_ISDIR(path_stat.st_mode);

    return true;
}

