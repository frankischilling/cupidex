// File: files.c
// -----------------------
#define _POSIX_C_SOURCE 200112L    // for strdup
#include <stdlib.h>                // for malloc, free
#include <stddef.h>                // for NULL
#include <sys/types.h>             // for ino_t
#include <string.h>                // for strdup
#include <dirent.h>                // for DIR, struct dirent, opendir, readdir, closedir
#include <unistd.h>                // for lstat
#include <stdio.h>                 // for snprintf
#include <sys/stat.h>              // for struct stat, lstat, S_ISDIR
#include <time.h>                  // for strftime
#include <curses.h>                // for WINDOW, mvwprintw
#include <stdbool.h>               // for bool, true, false
#include <string.h>                // for strcmp
#include <limits.h>                // For PATH_MAX
#include <fcntl.h>                 // For O_RDONLY
#include <magic.h>                 // For libmagic

// Local includes
#include "main.h"                  // for FileAttr, Vector, Vector_add, Vector_len, Vector_set_len
#include "utils.h"                 // for path_join, is_directory
#include "files.h"                 // for FileAttributes, FileAttr, MAX_PATH_LENGTH

#include <time.h>

#define MAX_PATH_LENGTH 1024
#define MIN_INT_SIZE_T(x, y) (((size_t)(x) > (y)) ? (y) : (x))
#define BANNER_UPDATE_INTERVAL 50000 // 50ms in microseconds

// Supported MIME types
const char *supported_mime_types[] = {
        "text/plain",             // Plain text files
        "text/x-c",               // C source files
        "application/json",       // JSON files
        "application/xml",        // XML files
        "text/x-shellscript",     // Shell scripts
        "text/x-python",          // Python source files
        "text/x-java-source",     // Java source files
        "text/html",              // HTML files
        "text/css",               // CSS files
        "text/x-c++src",          // C++ source files
        "application/x-yaml",     // YAML files
        "application/x-sh",       // Shell scripts
        "application/x-perl",     // Perl scripts
        "application/x-php",      // PHP scripts
        "text/x-rustsrc",         // Rust source files
        "text/x-go",              // Go source files
        "text/x-swift",           // Swift source files
        "text/x-kotlin",          // Kotlin source files
        "text/x-makefile",        // Makefile files
};  

size_t num_supported_mime_types = sizeof(supported_mime_types) / sizeof(supported_mime_types[0]);
// FileAttributes structure
struct FileAttributes {
    char *name;  //
    ino_t inode; // Change from int inode;
    bool is_dir;
};
// TextBuffer structure
typedef struct {
    char **lines;      // Dynamic array of strings
    int num_lines;     // Current number of lines
    int capacity;      // Total capacity of the array
} TextBuffer;
/**
 * Function to initialize a TextBuffer
 *
 * @param buffer the TextBuffer to initialize
 */
void init_text_buffer(TextBuffer *buffer) {
    buffer->capacity = 100; // Initial capacity
    buffer->num_lines = 0;
    buffer->lines = malloc(sizeof(char*) * buffer->capacity);
}
/**
 * Function to free the memory allocated for a TextBuffer
 *
 * @param buffer the TextBuffer to free
 */
const char *FileAttr_get_name(FileAttr fa) {
    if (fa != NULL) {
        return fa->name;
    } else {
        // Handle the case where fa is NULL
        return "Unknown";
    }
}
/**
 * Function to check if a file type is supported for preview
 *
 * @param filename the name of the file
 * @return true if the file type is supported, false otherwise
 */
bool FileAttr_is_dir(FileAttr fa) {
    if (fa == NULL) {
        return false; // Treat NULL as a non-directory
    }
    return fa->is_dir;
}
/**
 * Function to format a file size in a human-readable format
 *
 * @param buffer the buffer to store the formatted file size
 * @param size the size of the file
 * @return the formatted file size
 */
char* format_file_size(char *buffer, size_t size) {
    // iB for multiples of 1024, B for multiples of 1000
    // so, KiB = 1024, KB = 1000
    const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    int i = 0;
    double fileSize = (double)size;
    while (fileSize >= 1024 && i < 4) {
        fileSize /= 1024;
        i++;
    }
    sprintf(buffer, "%.2f %s", fileSize, units[i]);
    return buffer;
}
/**
 * Function to create a new FileAttr
 *
 * @param name the name of the file
 * @param is_dir true if the file is a directory, false otherwise
 * @param inode the inode number of the file
 * @return a new FileAttr
 */
FileAttr mk_attr(const char *name, bool is_dir, ino_t inode) {
    FileAttr fa = malloc(sizeof(struct FileAttributes));

    if (fa != NULL) {
        fa->name = strdup(name);

        if (fa->name == NULL) {
            // Handle memory allocation failure for the name
            free(fa);
            return NULL;
        }

        fa->inode = inode;
        fa->is_dir = is_dir;
        return fa;
    } else {
        // Handle memory allocation failure for the FileAttr
        return NULL;
    }
}
// Function to free the allocated memory for a FileAttr
void free_attr(FileAttr fa) {
    if (fa != NULL) {
        free(fa->name);  // Free the allocated memory for the name
        free(fa);
    }
}
/**
 * Function to append files in a directory to a Vector
 *
 * @param v the Vector to append the files to
 * @param name the name of the directory
 */
void append_files_to_vec(Vector *v, const char *name) {
    DIR *dir = opendir(name);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char full_path[MAX_PATH_LENGTH];
                path_join(full_path, name, entry->d_name);

                bool is_dir = is_directory(name, entry->d_name);

                FileAttr file_attr = mk_attr(entry->d_name, is_dir, entry->d_ino);

                if (file_attr != NULL) {  // Only add if not NULL
                    Vector_add(v, 1);
                    v->el[Vector_len(*v)] = file_attr;
                    Vector_set_len(v, Vector_len(*v) + 1);
                }
            }
        }
        closedir(dir);
    }
}

// Recursive function to calculate directory size
// NOTE: this function may take long, it might be better to have the size of
//       the directories displayed as "-" until we have a value. Use of "du" or
//       another already existent tool might be better. The sizes should
//       probably be cached.
//       fork() -> exec() (if using du)
//       fork() -> calculate directory size and return it somehow (maybe print
//       as binary to the stdout)
long get_directory_size(const char *dir_path) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    long total_size = 0;
    const long MAX_SIZE_THRESHOLD = 1000L * 1024 * 1024 * 1024 * 1024; // 1000 TiB

    if (!(dir = opendir(dir_path)))
        return -1;

    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATH_LENGTH];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        if (lstat(path, &statbuf) == -1)
            continue;
        if (S_ISDIR(statbuf.st_mode)) {
            long dir_size = get_directory_size(path);
            if (dir_size == -2) {
                closedir(dir);
                return -2;
            }
            total_size += dir_size;
        } else {
            total_size += statbuf.st_size;
        }
        if (total_size > MAX_SIZE_THRESHOLD) {
            closedir(dir);
            return -2;
        }
    }

    closedir(dir);
    return total_size;
}
/**
 * Function to display file information in a window
 *
 * @param window the window to display the file information
 * @param file_path the path to the file
 * @param max_x the maximum width of the window
 */
void display_file_info(WINDOW *window, const char *file_path, int max_x) {
    struct stat file_stat;

    // Attempt to retrieve file statistics
    if (stat(file_path, &file_stat) == -1) {
        mvwprintw(window, 2, 2, "Unable to retrieve file information");
        return;
    }

    // Define a fixed width for labels to ensure alignment
    int label_width = 20;

    // Display File Size or Directory Size
    if (S_ISDIR(file_stat.st_mode)) {
        long dir_size = get_directory_size(file_path);
        if (dir_size == -2) {
            mvwprintw(window, 2, 2, "%-*s %s", label_width, "Directory Size:", "Uncalculable");
        } else {
            char fileSizeStr[20];
            format_file_size(fileSizeStr, dir_size);
            mvwprintw(window, 2, 2, "%-*s %s", label_width, "Directory Size:", fileSizeStr);
        }
    } else {
        char fileSizeStr[20];
        format_file_size(fileSizeStr, file_stat.st_size);
        mvwprintw(window, 2, 2, "%-*s %s", label_width, "File Size:", fileSizeStr);
    }

    // Display File Permissions
    char permissions[22];
    snprintf(permissions, sizeof(permissions), "%o", file_stat.st_mode & 0777);
    mvwprintw(window, 3, 2, "%-*s %s", label_width, "File Permissions:", permissions);

    // Display Last Modification Time
    char modTime[50];
    strftime(modTime, sizeof(modTime), "%c", localtime(&file_stat.st_mtime));
    mvwprintw(window, 4, 2, "%-*s %.24s", label_width, "Last Modification Time:", modTime);

    // Display MIME type using libmagic
    magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
    if (magic_cookie == NULL) {
        mvwprintw(window, 5, 2, "%-*s %s", label_width, "MIME type:", "Error initializing magic library");
        return;
    }
    if (magic_load(magic_cookie, NULL) != 0) {
        mvwprintw(window, 5, 2, "%-*s %s", label_width, "MIME type:", magic_error(magic_cookie));
        magic_close(magic_cookie);
        return;
    }
    const char *mime_type = magic_file(magic_cookie, file_path);
    if (mime_type == NULL) {
        mvwprintw(window, 5, 2, "%-*s %s", label_width, "MIME type:", "Unknown (error)");
    } else {
        // Define value_width as size_t to match strlen's return type
        size_t value_width = (size_t)(max_x - 2 - label_width - 1); // 2 for left margin, 1 for space

        // Truncate MIME type string if it's too long
        if (strlen(mime_type) > value_width) {
            char truncated_mime[value_width + 1];
            strncpy(truncated_mime, mime_type, value_width);
            truncated_mime[value_width] = '\0';
            mvwprintw(window, 5, 2, "%-*s %s", label_width, "MIME type:", truncated_mime);
        } else {
            mvwprintw(window, 5, 2, "%-*s %s", label_width, "MIME type:", mime_type);
        }
    }
    magic_close(magic_cookie);
}
/**
 * Function to check if a file type is supported for preview
 *
 * @param filename the name of the file
 * @return true if the file type is supported, false otherwise
 */
void render_text_buffer(WINDOW *window, TextBuffer *buffer, int start_line, int cursor_line, int cursor_col) {
    werase(window);
    box(window, 0, 0);

    int max_y, max_x;
    getmaxyx(window, max_y, max_x);

    for (int i = 0; i < max_y - 2 && (start_line + i) < buffer->num_lines; i++) {
        const char *line = buffer->lines[start_line + i] ? buffer->lines[start_line + i] : ""; // Safely handle NULL
        if ((start_line + i) == cursor_line) {
            wattron(window, A_REVERSE);
        }
        mvwprintw(window, i + 1, 2, "%.*s", max_x - 4, line);
        if ((start_line + i) == cursor_line) {
            wattroff(window, A_REVERSE);
        }
    }

    // Move cursor to the appropriate position
    wmove(window, cursor_line - start_line + 1, cursor_col + 2);
    wrefresh(window);
}
/**
 * Function to edit a file in the terminal using a text buffer
 *
 * @param window the window to display the file content
 * @param file_path the path to the file to edit
 */
void edit_file_in_terminal(WINDOW *window, const char *file_path, WINDOW *notification_window) {
    // Store previous scroll time
    struct timespec prev_scroll_time = last_scroll_time;

    // Open the file for reading and writing
    int fd = open(file_path, O_RDWR);
    if (fd == -1) {
        mvwprintw(notification_window, 1, 2, "Unable to open file");
        wrefresh(notification_window);
        return;
    }

    FILE *file = fdopen(fd, "r+");
    if (file == NULL) {
        mvwprintw(notification_window, 1, 2, "Unable to open file stream");
        wrefresh(notification_window);
        close(fd);
        return;
    }

    // Clear the window before editing
    werase(window);
    box(window, 0, 0);

    int ch;
    TextBuffer text_buffer;
    init_text_buffer(&text_buffer);

    char line[256];
    bool is_empty = true;

    // Read the file content into the text buffer
    while (fgets(line, sizeof(line), file)) {
        is_empty = false;
        line[strcspn(line, "\n")] = '\0';

        // Replace tabs with spaces
        for (char *p = line; *p; p++) {
            if (*p == '\t') {
                *p = ' ';
            }
        }

        if (text_buffer.num_lines >= text_buffer.capacity) {
            text_buffer.capacity *= 2;
            text_buffer.lines = realloc(text_buffer.lines, sizeof(char*) * text_buffer.capacity);
            if (!text_buffer.lines) {
                mvwprintw(notification_window, 1, 2, "Memory allocation error");
                wrefresh(notification_window);
                fclose(file);
                return;
            }
        }

        text_buffer.lines[text_buffer.num_lines++] = strdup(line);
    }

    if (is_empty) {
        text_buffer.lines[text_buffer.num_lines++] = strdup("");
    }

    // Call render_text_buffer to display the initial content
    render_text_buffer(window, &text_buffer, 0, 0, 0);

    // Enable cursor and allow editing
    curs_set(1);
    keypad(window, TRUE);
    wmove(window, 1, 2);

    int cursor_line = 0;
    int cursor_col = 0;
    int start_line = 0;

    bool exit_edit_mode = false;
    
    // Set window timeout for non-blocking input
    wtimeout(window, 10);  // Short timeout for responsive input

    while (!exit_edit_mode) {
        ch = wgetch(window);
        
        if (ch == ERR) {
            napms(10);
            continue;
        }

        if (ch == KEY_F(1)) break;

        int max_y;
        getmaxyx(window, max_y, max_y);

        switch (ch) {
            case KEY_UP:
                if (cursor_line > 0) {
                    cursor_line--;
                    if (cursor_line < start_line) {
                        start_line--;
                    }
                    cursor_col = MIN(cursor_col, (int)strlen(text_buffer.lines[cursor_line]));
                }
                break;
            case KEY_DOWN:
                if (cursor_line < text_buffer.num_lines - 1) {
                    cursor_line++;
                    if (cursor_line - start_line >= max_y - 2) {
                        start_line = cursor_line - (max_y - 3);
                    }
                    cursor_col = MIN(cursor_col, (int)strlen(text_buffer.lines[cursor_line]));
                }
                break;
            case KEY_LEFT:
                if (cursor_col > 0) {
                    cursor_col--;
                } else if (cursor_line > 0) {
                    cursor_line--;
                    cursor_col = (int)strlen(text_buffer.lines[cursor_line]);
                    if (cursor_line < start_line) {
                        start_line--;
                    }
                }
                break;
            case KEY_RIGHT:
                if (cursor_col < (int)strlen(text_buffer.lines[cursor_line])) {
                    cursor_col++;
                } else if (cursor_line < text_buffer.num_lines - 1) {
                    cursor_line++;
                    cursor_col = 0;
                    if (cursor_line - start_line >= max_y - 2) {
                        start_line++;
                    }
                }
                break;
            case '\n': {
                char *current_line = text_buffer.lines[cursor_line];
                char *new_line = strdup(current_line + cursor_col);
                current_line[cursor_col] = '\0';

                if (text_buffer.num_lines >= text_buffer.capacity) {
                    text_buffer.capacity *= 2;
                    text_buffer.lines = realloc(text_buffer.lines, sizeof(char*) * text_buffer.capacity);
                    if (!text_buffer.lines) {
                        mvwprintw(notification_window, 1, 2, "Memory allocation error");
                        wrefresh(notification_window);
                        fclose(file);
                        return;
                    }
                }

                for (int i = text_buffer.num_lines; i > cursor_line + 1; i--) {
                    text_buffer.lines[i] = text_buffer.lines[i - 1];
                }
                text_buffer.lines[cursor_line + 1] = new_line;
                text_buffer.num_lines++;

                cursor_line++;
                cursor_col = 0;
                if (cursor_line - start_line >= max_y - 2) {
                    start_line++;
                }
            }
                break;
            case KEY_BACKSPACE:
            case 127:
                if (cursor_col > 0) {
                    char *current_line = text_buffer.lines[cursor_line];
                    memmove(&current_line[cursor_col - 1], &current_line[cursor_col], strlen(current_line) - cursor_col + 1);
                    cursor_col--;
                } else if (cursor_line > 0) {
                    int prev_len = (int)strlen(text_buffer.lines[cursor_line - 1]);
                    int curr_len = (int)strlen(text_buffer.lines[cursor_line]);

                    text_buffer.lines[cursor_line - 1] = realloc(text_buffer.lines[cursor_line - 1], prev_len + curr_len + 1);
                    strcat(text_buffer.lines[cursor_line - 1], text_buffer.lines[cursor_line]);
                    free(text_buffer.lines[cursor_line]);

                    for (int i = cursor_line; i < text_buffer.num_lines - 1; i++) {
                        text_buffer.lines[i] = text_buffer.lines[i + 1];
                    }
                    text_buffer.num_lines--;

                    cursor_line--;
                    cursor_col = prev_len;
                    if (cursor_line < start_line) {
                        start_line--;
                    }
                }
                break;
            case 7:  // Ctrl+G
                fclose(file);
                file = fopen(file_path, "w");
                if (file == NULL) {
                    mvwprintw(notification_window, LINES - 2, 2, "Error opening file for writing");
                    wrefresh(notification_window);
                    break;
                }

                for (int i = 0; i < text_buffer.num_lines; i++) {
                    if (fprintf(file, "%s\n", text_buffer.lines[i]) < 0) {
                        mvwprintw(notification_window, LINES - 2, 2, "Error writing to file");
                        wrefresh(notification_window);
                        break;
                    }
                }

                fflush(file);
                fclose(file);
                file = fopen(file_path, "r+");
                if (file == NULL) {
                    mvwprintw(notification_window, LINES - 2, 2, "Error reopening file");
                    wrefresh(notification_window);
                    break;
                }

                werase(notification_window);
                mvwprintw(notification_window, 0, 0, "File saved: %s", file_path);
                wrefresh(notification_window);
                break;
            case 5:   // Ctrl+E
                exit_edit_mode = true;
                break;
            default:
                if (ch >= 32 && ch <= 126) {
                    char *current_line = text_buffer.lines[cursor_line];
                    int line_len = (int)strlen(current_line);
                    current_line = realloc(current_line, line_len + 2);
                    memmove(&current_line[cursor_col + 1], &current_line[cursor_col], line_len - cursor_col + 1);
                    current_line[cursor_col] = ch;
                    text_buffer.lines[cursor_line] = current_line;
                    cursor_col++;
                }
                break;
        }

        // Render the updated buffer
        render_text_buffer(window, &text_buffer, start_line, cursor_line, cursor_col);
    }

    fclose(file);
    curs_set(0);

    // Reset window timeout to blocking mode
    wtimeout(window, -1);

    // Restore banner scrolling state
    last_scroll_time = prev_scroll_time;

    // Clean up
    for (int i = 0; i < text_buffer.num_lines; i++) {
        free(text_buffer.lines[i]);
    }
    free(text_buffer.lines);
}

/**
 * Checks if the given file has a supported MIME type.
 *
 * @param filename The name of the file to check.
 * @return true if the file has a supported MIME type, false otherwise.
 */
bool is_supported_file_type(const char *filename) {
    magic_t magic_cookie;
    bool supported = false;

    // Open a magic cookie for checking MIME types
    magic_cookie = magic_open(MAGIC_MIME_TYPE);
    if (magic_cookie == NULL) {
        fprintf(stderr, "Unable to initialize magic library\n");
        return false;
    }

    // Load the default magic database
    if (magic_load(magic_cookie, NULL) != 0) {
        fprintf(stderr, "Cannot load magic database: %s\n", magic_error(magic_cookie));
        magic_close(magic_cookie);
        return false;
    }

    // Get the MIME type of the file
    const char *mime_type = magic_file(magic_cookie, filename);
    if (mime_type == NULL) {
        fprintf(stderr, "Could not determine file type: %s\n", magic_error(magic_cookie));
        magic_close(magic_cookie);
        return false;
    }

    // Check if MIME type is in supported types
    for (size_t i = 0; i < num_supported_mime_types; i++) {
        if (strcmp(mime_type, supported_mime_types[i]) == 0) {
            supported = true;
            break;
        }
    }

    // Clean up
    magic_close(magic_cookie);
    return supported;
}