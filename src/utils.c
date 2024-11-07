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
/**
 * Function to join two paths together
 *
 * @param result the resulting path
 * @param base the base path
 * @param extra the extra path to append
 */
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
/**
 * Function to join two paths together
 *
 * @param result the resulting path
 * @param base the base path
 * @param extra the extra path to append
 */
void create_file(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (f == NULL)
        die(1, "Couldn't create file %s", filename);
    fclose(f);
}
/**
 * Function to join two paths together
 *
 * @param result the resulting path
 * @param base the base path
 * @param extra the extra path to append
 */
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
/**
 * Function to display the contents of a directory in the terminal
 *
 * @param directory the directory to display
 */
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
/**
 * Function to preview the contents of a file in the terminal
 *
 * @param filename the name of the file to preview
 */
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
/**
 * Function to change the current directory and update the list of files
 *
 * @param new_directory the new directory to change to
 * @param files the list of files in the directory
 * @param num_files the number of files in the directory
 * @param selected_entry the index of the selected entry
 * @param start_entry the index of the first entry displayed
 * @param end_entry the index of the last entry displayed
 */
bool is_directory(const char *path, const char *filename) {
    struct stat path_stat;
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    if (stat(full_path, &path_stat) == 0)
        return S_ISDIR(path_stat.st_mode);

    return false; // Correct: Do not assume it's a directory if stat fails
}

/** Function to join two paths
 *
 * @param result the resulting path
 * @param base the base path
 * @param extra the extra path
 */
void path_join(char *result, const char *base, const char *extra) {
    size_t base_len = strlen(base);
    size_t extra_len = strlen(extra);

    if (base_len == 0) {
        strncpy(result, extra, MAX_PATH_LENGTH);
    } else if (extra_len == 0) {
        strncpy(result, base, MAX_PATH_LENGTH);
    } else {
        if (base[base_len - 1] == '/') {
            snprintf(result, MAX_PATH_LENGTH, "%s%s", base, extra);
        } else {
            snprintf(result, MAX_PATH_LENGTH, "%s/%s", base, extra);
        }
    }

    result[MAX_PATH_LENGTH - 1] = '\0';
}