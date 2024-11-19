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
#include <ctype.h>     // for isprint

// Local includes
#include "utils.h"
#include "files.h"  // Include the header for FileAttr and related functions

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
    printf("Attempting to preview file: %s\n", filename);
    
    // First check if file exists and is readable
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Couldn't open file %s for preview\n", filename);
        printf("Errno: %d - %s\n", errno, strerror(errno));
        return;
    }

    // Initialize ncurses
    initscr();
    start_color();
    noecho();
    keypad(stdscr, TRUE);
    raw();
    
    int ch;
    int row = 0;
    int col = 0;
    int max_rows, max_cols;
    
    getmaxyx(stdscr, max_rows, max_cols);
    
    // Clear screen and show header
    clear();
    attron(A_BOLD | A_REVERSE);
    printw("File Preview: %s", filename);
    attroff(A_BOLD | A_REVERSE);
    printw("\nPress 'q' to exit, arrow keys to scroll\n\n");
    refresh();
    
    row = 3;
    
    // Read and display file contents
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        col = 0;
        for (int i = 0; buffer[i] != '\0' && col < max_cols - 1; i++) {
            if (buffer[i] == '\t') {
                // Handle tabs
                for (int j = 0; j < 4 && col < max_cols - 1; j++) {
                    mvaddch(row, col++, ' ');
                }
            } else if (isprint(buffer[i]) || buffer[i] == '\n') {
                // Handle printable characters
                mvaddch(row, col++, buffer[i]);
            } else {
                // Handle non-printable characters
                mvaddch(row, col++, '?');
            }
        }
        row++;
        
        if (row >= max_rows - 1) {
            break;
        }
    }
    
    refresh();
    
    // Wait for 'q' to exit
    while ((ch = getch()) != 'q');
    
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

/**
 * Function to get an emoji based on the file's MIME type.
 *
 * @param mime_type The MIME type of the file.
 * @return A string representing the emoji.
 */
const char* get_file_emoji(const char *mime_type) {
    if (mime_type == NULL) {
        return "📄";
    }

    // Text files
    if (strncmp(mime_type, "text/", 5) == 0) {
        if (strstr(mime_type, "python")) return "🐍";
        if (strstr(mime_type, "javascript")) return "📜";
        if (strstr(mime_type, "html")) return "🌐";
        if (strstr(mime_type, "css")) return "🎨";
        if (strstr(mime_type, "x-c")) return "📝";
        if (strstr(mime_type, "x-java")) return "☕";
        if (strstr(mime_type, "x-shellscript")) return "💻";
        if (strstr(mime_type, "x-rust")) return "🦀";
        if (strstr(mime_type, "markdown")) return "📘";
        if (strstr(mime_type, "csv")) return "📊";
        if (strstr(mime_type, "x-perl")) return "🐪";
        if (strstr(mime_type, "x-ruby")) return "💎";
        if (strstr(mime_type, "x-php")) return "🐘";
        if (strstr(mime_type, "x-go")) return "🐹";
        if (strstr(mime_type, "x-swift")) return "🦅";
        if (strstr(mime_type, "x-kotlin")) return "🎯";
        if (strstr(mime_type, "x-scala")) return "⚡";
        if (strstr(mime_type, "x-haskell")) return "λ";
        if (strstr(mime_type, "x-lua")) return "🌙";
        if (strstr(mime_type, "x-r")) return "📊";
        
        // Data formats
        if (strstr(mime_type, "json")) return "🔣";
        if (strstr(mime_type, "xml")) return "📑";
        if (strstr(mime_type, "yaml")) return "📋";
        if (strstr(mime_type, "toml")) return "⚙️";
        if (strstr(mime_type, "ini")) return "🔧";
        return "📄";
    }

    // Images
    if (strncmp(mime_type, "image/", 6) == 0) {
        if (strstr(mime_type, "gif")) return "🎭";
        if (strstr(mime_type, "svg")) return "✨";
        if (strstr(mime_type, "png")) return "🖼️";
        if (strstr(mime_type, "jpeg") || strstr(mime_type, "jpg")) return "📸";
        if (strstr(mime_type, "webp")) return "🌅";
        if (strstr(mime_type, "tiff")) return "📷";
        if (strstr(mime_type, "bmp")) return "🎨";
        if (strstr(mime_type, "ico")) return "🎯";
        return "🖼️";
    }

    // Audio
    if (strncmp(mime_type, "audio/", 6) == 0) {
        if (strstr(mime_type, "midi")) return "🎹";
        if (strstr(mime_type, "mp3")) return "🎵";
        if (strstr(mime_type, "wav")) return "🔊";
        if (strstr(mime_type, "ogg")) return "🎼";
        if (strstr(mime_type, "flac")) return "🎶";
        if (strstr(mime_type, "aac")) return "🔉";
        return "🎵";
    }

    // Video
    if (strncmp(mime_type, "video/", 6) == 0) {
        if (strstr(mime_type, "mp4")) return "🎥";
        if (strstr(mime_type, "avi")) return "📽️";
        if (strstr(mime_type, "mkv")) return "🎬";
        if (strstr(mime_type, "webm")) return "▶️";
        if (strstr(mime_type, "mov")) return "🎦";
        if (strstr(mime_type, "wmv")) return "📹";
        return "🎞️";
    }

    // Applications
    if (strncmp(mime_type, "application/", 12) == 0) {
        // Archives
        if (strstr(mime_type, "zip")) return "📦";
        if (strstr(mime_type, "x-tar")) return "📦";
        if (strstr(mime_type, "x-rar")) return "📦";
        if (strstr(mime_type, "x-7z")) return "📦";
        if (strstr(mime_type, "gzip")) return "📦";
        if (strstr(mime_type, "x-bzip")) return "📦";
        if (strstr(mime_type, "x-xz")) return "📦";
        if (strstr(mime_type, "x-compress")) return "📦";

        // Documents
        if (strstr(mime_type, "pdf")) return "📕";
        if (strstr(mime_type, "msword")) return "📝";
        if (strstr(mime_type, "vnd.ms-excel")) return "📊";
        if (strstr(mime_type, "vnd.ms-powerpoint")) return "📊";
        if (strstr(mime_type, "vnd.oasis.opendocument.text")) return "📃";
        if (strstr(mime_type, "rtf")) return "📄";
        if (strstr(mime_type, "epub")) return "📚";
        if (strstr(mime_type, "js")) return "📜";

        // Data formats
        if (strstr(mime_type, "json")) return "🔣";
        if (strstr(mime_type, "xml")) return "📑";
        if (strstr(mime_type, "yaml")) return "📋";
        if (strstr(mime_type, "sql")) return "🗄️";
        
        // Executables and binaries
        if (strstr(mime_type, "x-executable")) return "⚙️";
        if (strstr(mime_type, "x-sharedlib")) return "🔧";
        if (strstr(mime_type, "x-object")) return "🔨";
        if (strstr(mime_type, "x-pie-executable")) return "🎯";
        if (strstr(mime_type, "x-dex")) return "🤖";
        if (strstr(mime_type, "java-archive")) return "☕";
        if (strstr(mime_type, "x-msdownload")) return "🪟";
    }

    // Font files
    if (strncmp(mime_type, "font/", 5) == 0) {
        if (strstr(mime_type, "ttf")) return "🔤";
        if (strstr(mime_type, "otf")) return "🔠";
        if (strstr(mime_type, "woff")) return "🔡";
        if (strstr(mime_type, "woff2")) return "🔣";
        return "🔤";
    }

    // Database files
    if (strstr(mime_type, "database") || strstr(mime_type, "sql")) {
        return "🗄️";
    }

    // Version control
    if (strstr(mime_type, "x-git")) {
        return "📥";
    }

    // Certificate files
    if (strstr(mime_type, "x-x509-ca-cert")) {
        return "🔐";
    }

    return "📄";
}