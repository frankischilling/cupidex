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
#include <libgen.h>    // for dirname() and basename()

// Local includes
#include "utils.h"
#include "files.h"  // Include the header for FileAttr and related functions
#include "globals.h"

#define MAX_DISPLAY_LENGTH 32

// Declare copied_filename as a global variable at the top of the file
char copied_filename[MAX_PATH_LENGTH] = "";

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
const char* get_file_emoji(const char *mime_type, const char *filename) {
    if (mime_type == NULL) {
        return "📄";
    }

    // Check for specific MIME types first
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
    }

    // Fallback to extension-based detection if MIME type is "text/plain"
    if (strcmp(mime_type, "text/plain") == 0) {
        const char *ext = strrchr(filename, '.');
        if (ext) {
            if (strcmp(ext, ".js") == 0) return "📜";
            if (strcmp(ext, ".py") == 0) return "🐍";
            if (strcmp(ext, ".html") == 0) return "🌐";
            if (strcmp(ext, ".css") == 0) return "🎨";
            if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) return "📝";
            if (strcmp(ext, ".java") == 0) return "☕";
            if (strcmp(ext, ".sh") == 0) return "💻";
            if (strcmp(ext, ".rs") == 0) return "🦀";
            if (strcmp(ext, ".md") == 0) return "📘";
            if (strcmp(ext, ".csv") == 0) return "📊";
            if (strcmp(ext, ".pl") == 0) return "🐪";
            if (strcmp(ext, ".rb") == 0) return "💎";
            if (strcmp(ext, ".php") == 0) return "🐘";
            if (strcmp(ext, ".go") == 0) return "🐹";
            if (strcmp(ext, ".swift") == 0) return "🦅";
            if (strcmp(ext, ".kt") == 0) return "🎯";
            if (strcmp(ext, ".scala") == 0) return "⚡";
            if (strcmp(ext, ".hs") == 0) return "λ";
            if (strcmp(ext, ".lua") == 0) return "🌙";
            if (strcmp(ext, ".r") == 0) return "📊";
            if (strcmp(ext, ".json") == 0) return "🔣";
            if (strcmp(ext, ".xml") == 0) return "📑";
            if (strcmp(ext, ".yaml") == 0 || strcmp(ext, ".yml") == 0) return "📋";
            if (strcmp(ext, ".toml") == 0) return "⚙️";
            if (strcmp(ext, ".ini") == 0) return "🔧";
        }
    }

    // Images
    if (strncmp(mime_type, "image/", 6) == 0) {
        if (strstr(mime_type, "gif")) return "🎭";
        if (strstr(mime_type, "svg")) return "✨";
        if (strstr(mime_type, "png")) return "🖼️ ";
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

    // Fallback to extension-based detection
    const char *ext = strrchr(filename, '.');
    if (ext) {
        if (strcmp(ext, ".py") == 0) return "🐍";
        if (strcmp(ext, ".js") == 0) return "📜";
        if (strcmp(ext, ".html") == 0) return "🌐";
        if (strcmp(ext, ".css") == 0) return "🎨";
        if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) return "📝";
        if (strcmp(ext, ".java") == 0) return "☕";
        if (strcmp(ext, ".sh") == 0) return "💻";
        if (strcmp(ext, ".rs") == 0) return "🦀";
        if (strcmp(ext, ".md") == 0) return "📘";
        if (strcmp(ext, ".csv") == 0) return "📊";
        if (strcmp(ext, ".pl") == 0) return "🐪";
        if (strcmp(ext, ".rb") == 0) return "💎";
        if (strcmp(ext, ".php") == 0) return "🐘";
        if (strcmp(ext, ".go") == 0) return "🐹";
        if (strcmp(ext, ".swift") == 0) return "🦅";
        if (strcmp(ext, ".kt") == 0) return "🎯";
        if (strcmp(ext, ".scala") == 0) return "⚡";
        if (strcmp(ext, ".hs") == 0) return "λ";
        if (strcmp(ext, ".lua") == 0) return "🌙";
        if (strcmp(ext, ".r") == 0) return "📊";
        if (strcmp(ext, ".json") == 0) return "🔣";
        if (strcmp(ext, ".xml") == 0) return "📑";
        if (strcmp(ext, ".yaml") == 0 || strcmp(ext, ".yml") == 0) return "📋";
        if (strcmp(ext, ".toml") == 0) return "⚙️";
        if (strcmp(ext, ".ini") == 0) return "🔧";
    }

    return "📄";
}

// copy file to users clipboard
void copy_to_clipboard(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        fprintf(stderr, "Error: Unable to get file/directory stats.\n");
        return;
    }

    // Create a temporary file to store path
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "/tmp/cupidfm_copy_%d", getpid());
    
    FILE *temp = fopen(temp_path, "w");
    if (!temp) {
        fprintf(stderr, "Error: Unable to create temporary file.\n");
        return;
    }

    // Write the path and whether it's a directory
    fprintf(temp, "%s\n%d", path, S_ISDIR(path_stat.st_mode));
    fclose(temp);

    // Copy the temp file content to clipboard
    char command[1024];
    snprintf(command, sizeof(command), "xclip -selection clipboard -i < %s", temp_path);
    
    int result = system(command);
    if (result == -1) {
        fprintf(stderr, "Error: Unable to copy to clipboard.\n");
    }

    // Clean up
    unlink(temp_path);
}

// paste files to directory the user in
void paste_from_clipboard(const char *target_directory, const char *filename) {
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "/tmp/cupidfm_paste_%d", getpid());
    
    char command[1024];
    snprintf(command, sizeof(command), "xclip -selection clipboard -o > %s", temp_path);
    
    if (system(command) == -1) {
        fprintf(stderr, "Error: Unable to read from clipboard.\n");
        return;
    }

    FILE *temp = fopen(temp_path, "r");
    if (!temp) {
        unlink(temp_path);
        return;
    }

    char source_path[512];
    int is_directory;
    char operation[10] = {0};
    
    if (fscanf(temp, "%511[^\n]\n%d\n%9s", source_path, &is_directory, operation) < 2) {
        fclose(temp);
        unlink(temp_path);
        return;
    }
    fclose(temp);
    unlink(temp_path);

    // Check if this is a cut operation
    bool is_cut = (operation[0] == 'C' && operation[1] == 'U' && operation[2] == 'T');

    if (is_cut) {
        // Get the temporary storage path
        char temp_storage[MAX_PATH_LENGTH];
        snprintf(temp_storage, sizeof(temp_storage), "/tmp/cupidfm_cut_storage_%d", getpid());
        
        // Move from temporary storage to target
        char mv_command[2048];
        snprintf(mv_command, sizeof(mv_command), "mv \"%s\" \"%s/%s\"", 
                temp_storage, target_directory, filename);
        
        if (system(mv_command) == -1) {
            fprintf(stderr, "Error: Unable to move file from temporary storage.\n");
            return;
        }
    } else {
        // Handle regular copy operation as before
        char cp_command[2048];
        snprintf(cp_command, sizeof(cp_command), "cp %s \"%s\" \"%s/%s\"",
                is_directory ? "-r" : "", source_path, target_directory, filename);
        
        if (system(cp_command) == -1) {
            fprintf(stderr, "Error: Unable to copy file.\n");
        }
    }
}

// cut and put into memory 
void cut_and_paste(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        fprintf(stderr, "Error: Unable to get file/directory stats.\n");
        return;
    }

    // Create a temporary file to store path with CUT flag
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "/tmp/cupidfm_cut_%d", getpid());
    
    FILE *temp = fopen(temp_path, "w");
    if (!temp) {
        fprintf(stderr, "Error: Unable to create temporary file.\n");
        return;
    }

    // Write the path, whether it's a directory, and mark it as CUT operation
    fprintf(temp, "%s\n%d\nCUT", path, S_ISDIR(path_stat.st_mode));
    fclose(temp);

    // Copy the temp file content to clipboard
    char command[1024];
    snprintf(command, sizeof(command), "xclip -selection clipboard -i < %s", temp_path);
    
    int result = system(command);
    if (result == -1) {
        fprintf(stderr, "Error: Unable to copy to clipboard.\n");
    }

    // Clean up
    unlink(temp_path);

    // Hide the file from the current view by moving it to a temporary location
    char temp_storage[MAX_PATH_LENGTH];
    snprintf(temp_storage, sizeof(temp_storage), "/tmp/cupidfm_cut_storage_%d", getpid());
    
    // Move the file to temporary storage
    char mv_command[2048];
    snprintf(mv_command, sizeof(mv_command), "mv \"%s\" \"%s\"", path, temp_storage);
    
    if (system(mv_command) == -1) {
        fprintf(stderr, "Error: Unable to move file to temporary storage.\n");
        return;
    }
}

void delete_item(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        fprintf(stderr, "Error: Unable to get file/directory stats.\n");
        return;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        // For directories, use rm -rf command
        char command[1024];
        snprintf(command, sizeof(command), "rm -rf \"%s\"", path);
        
        int result = system(command);
        if (result == -1) {
            fprintf(stderr, "Error: Unable to delete directory: %s\n", path);
        }
    } else {
        // For regular files, use unlink
        if (unlink(path) != 0) {
            fprintf(stderr, "Error: Unable to delete file: %s\n", path);
        }
    }
}

void confirm_delete(WINDOW *notifwin, const char *path, bool *should_delete) {
    *should_delete = false;
    
    // Show confirmation prompt
    werase(notifwin);
    mvwprintw(notifwin, 0, 0, "Delete '%s'? (y/n): ", path);
    wrefresh(notifwin);
    
    // Wait for user input
    int ch;
    while ((ch = wgetch(notifwin)) != ERR) {
        ch = tolower(ch);
        if (ch == 'y') {
            *should_delete = true;
            break;
        } else if (ch == 'n') {
            break;
        }
    }
}

/**
 * Rename a file or directory by prompting the user for a new name.
 *
 * @param notifwin    The ncurses window to display prompts and notifications.
 * @param old_path    The full path to the existing file or directory.
 */
void rename_item(WINDOW *notifwin, const char *old_path)
{
    // We'll extract the parent directory from old_path, 
    // then ask for the new filename (not the full path).
    // Finally, we join them together and call rename().
    
    // 1. Extract the directory portion from old_path
    //    (if old_path is "/some/path/file.txt",
    //     dirname_part will be "/some/path",
    //     basename_part will be "file.txt").
    char temp_path[MAX_PATH_LENGTH];
    strncpy(temp_path, old_path, sizeof(temp_path));
    temp_path[sizeof(temp_path) - 1] = '\0';
    char *dir_part  = dirname(temp_path);  // modifies temp_path internally

    // We need a copy for the basename because dirname() and basename() 
    // can interfere with each other if using the same buffer.
    char temp_path2[MAX_PATH_LENGTH];
    strncpy(temp_path2, old_path, sizeof(temp_path2));
    temp_path2[sizeof(temp_path2) - 1] = '\0';
    char *base_part = basename(temp_path2);

    // 2. Prompt user for the new name
    werase(notifwin);
    mvwprintw(notifwin, 0, 0, "Rename '%s' to: ", base_part);
    wclrtoeol(notifwin); // clear to end of line
    wrefresh(notifwin);

    // We will temporarily enable echo just for user input
    echo();
    curs_set(1);

    char new_name[MAX_PATH_LENGTH];
    memset(new_name, 0, sizeof(new_name));

    // Collect the user input on notifwin
    wgetnstr(notifwin, new_name, sizeof(new_name) - 1);

    // Restore noecho, hide cursor
    noecho();
    curs_set(0);

    // If user presses ESC or empty name, we can handle that here
    if (strlen(new_name) == 0) {
        werase(notifwin);
        mvwprintw(notifwin, 0, 0, "Rename cancelled.");
        wrefresh(notifwin);
        return;
    }

    // 3. Construct the new full path
    char new_full_path[MAX_PATH_LENGTH];
    path_join(new_full_path, dir_part, new_name);

    // 4. Perform the rename
    if (rename(old_path, new_full_path) == 0) {
        // success
        werase(notifwin);
        mvwprintw(notifwin, 0, 0, "Renamed '%s' -> '%s'", base_part, new_name);
        wrefresh(notifwin);
    } else {
        // error
        werase(notifwin);
        mvwprintw(notifwin, 0, 0, "Error renaming '%s': %s", base_part, strerror(errno));
        wrefresh(notifwin);
    }
}

/**
 * Create a new file in the current directory by prompting the user for a filename.
 *
 * @param notifwin        The ncurses window used to prompt/notify.
 * @param current_dir     The current directory path (e.g. "/home/user/...").
 */
void create_new_file(WINDOW *notifwin, const char *current_dir)
{
    // Prompt user for the new file name in notifwin
    werase(notifwin);
    mvwprintw(notifwin, 0, 0, "Enter new file name: ");
    wclrtoeol(notifwin);
    wrefresh(notifwin);

    // Temporarily enable echo + show cursor while user types
    echo();
    curs_set(1);

    char new_name[MAX_PATH_LENGTH];
    memset(new_name, 0, sizeof(new_name));
    wgetnstr(notifwin, new_name, sizeof(new_name) - 1);

    // Restore noecho + hide cursor
    noecho();
    curs_set(0);

    if (strlen(new_name) == 0) {
        // If user cancelled or typed an empty string
        werase(notifwin);
        mvwprintw(notifwin, 0, 0, "File creation cancelled.");
        wrefresh(notifwin);
        return;
    }

    // Join current_dir and new_name
    char full_path[MAX_PATH_LENGTH];
    path_join(full_path, current_dir, new_name);

    // Attempt to create the file
    FILE *f = fopen(full_path, "w");
    if (!f) {
        werase(notifwin);
        mvwprintw(notifwin, 0, 0, "Error creating file '%s': %s", new_name, strerror(errno));
        wrefresh(notifwin);
        return;
    }
    fclose(f);

    // Notify success
    werase(notifwin);
    mvwprintw(notifwin, 0, 0, "Created file: %s", new_name);
    wrefresh(notifwin);
}