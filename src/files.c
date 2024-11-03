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
#include <main.h>                  // for FileAttr, Vector, Vector_add, Vector_len, Vector_set_len
#include <utils.h>                 // for path_join, is_directory
#include <files.h>                 // for FileAttributes, FileAttr, MAX_PATH_LENGTH
#include <curses.h>                // for WINDOW, mvwprintw
#include <stdbool.h>               // for bool, true, false
#include <string.h>                // for strcmp
#include <limits.h>                // For PATH_MAX
#include <fcntl.h>                 // For O_RDONLY

#define MAX_PATH_LENGTH 1024
#define MIN_INT_SIZE_T(x, y) (((size_t)(x) > (y)) ? (y) : (x))


struct FileAttributes {
    char *name;  // Change from char name*;
    ino_t inode;
    bool is_dir;
};

typedef struct {
    char **lines;      // Dynamic array of strings
    int num_lines;     // Current number of lines
    int capacity;      // Total capacity of the array
} TextBuffer;

void init_text_buffer(TextBuffer *buffer) {
    buffer->capacity = 100; // Initial capacity
    buffer->num_lines = 0;
    buffer->lines = malloc(sizeof(char*) * buffer->capacity);
}

const char *FileAttr_get_name(FileAttr fa) {
    if (fa != NULL) {
        return fa->name;
    } else {
        // Handle the case where fa is NULL
        return "Unknown";
    }
}

bool FileAttr_is_dir(FileAttr fa) {
    return fa->is_dir;
}

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

void free_attr(FileAttr fa) {
    if (fa != NULL) {
        free(fa->name);  // Free the allocated memory for the name
        free(fa);
    }
}

void append_files_to_vec(Vector *v, const char *name) {
    DIR *dir = opendir(name);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Filter out "." and ".." entries
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char full_path[MAX_PATH_LENGTH];
                path_join(full_path, name, entry->d_name);

                bool is_dir = is_directory(name, entry->d_name);

                // Allocate memory for the FileAttr object
                FileAttr file_attr = mk_attr(entry->d_name, is_dir, entry->d_ino);

                // Add the FileAttr object to the vector
                Vector_add(v, 1);
                v->el[Vector_len(*v)] = file_attr;

                // Update the vector length
                Vector_set_len(v, Vector_len(*v) + 1);
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
// Function to display file information
void display_file_info(WINDOW *window, const char *file_path, int max_x) {
    struct stat file_stat;

    if (stat(file_path, &file_stat) == -1) {
        mvwprintw(window, 2, 2, "Unable to retrieve file information");
        return;
    }

    if (S_ISDIR(file_stat.st_mode)) {
        long dir_size = get_directory_size(file_path);
        if (dir_size == -2) {
            mvwprintw(window, 2, 2, "Directory Size: Uncalcable");
        } else {
            char fileSizeStr[20];
            mvwprintw(window, 2, 2, "Directory Size: %.*s", max_x - 4, format_file_size(fileSizeStr, dir_size));
        }
    } else {
        char fileSizeStr[20];
        mvwprintw(window, 2, 2, "File Size: %.*s", max_x - 4, format_file_size(fileSizeStr, file_stat.st_size));
    }

    char permissions[22];
    sprintf(permissions, "File Permissions: %o", file_stat.st_mode & 0777);
    mvwprintw(window, 3, 2, "%.*s", max_x - 4, permissions);

    char modTime[50];
    strftime(modTime, sizeof(modTime), "%c", localtime(&file_stat.st_mtime));
    mvwprintw(window, 4, 2, "Last Modification Time: %.24s", modTime);
}

void render_text_buffer(WINDOW *window, TextBuffer *buffer, int start_line, int cursor_line, int cursor_col) {
    werase(window);
    box(window, 0, 0);

    int max_y, max_x;
    getmaxyx(window, max_y, max_x);

    for (int i = 0; i < max_y - 2 && (start_line + i) < buffer->num_lines; i++) {
        // Highlight the current line
        if ((start_line + i) == cursor_line) {
            wattron(window, A_REVERSE);
        }
        mvwprintw(window, i + 1, 2, "%.*s", max_x - 4, buffer->lines[start_line + i]);
        if ((start_line + i) == cursor_line) {
            wattroff(window, A_REVERSE);
        }
    }

    // Move cursor to the appropriate position
    wmove(window, cursor_line - start_line + 1, cursor_col + 2);
    wrefresh(window);
}

void edit_file_in_terminal(WINDOW *window, const char *file_path, WINDOW *notifwin) {
    int fd = open(file_path, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        mvwprintw(notifwin, 1, 2, "Unable to open file for editing");
        wrefresh(notifwin);
        return;
    }

    FILE *file = fdopen(fd, "r+");
    if (file == NULL) {
        mvwprintw(notifwin, 1, 2, "Unable to open file stream");
        wrefresh(notifwin);
        close(fd);
        return;
    }

    // Clear the window before editing
    werase(window);
    box(window, 0, 0);

    int ch;
    // int row = 1; // Remove this line since 'row' is unused

    TextBuffer text_buffer;
    init_text_buffer(&text_buffer);

    char line[256];

    // Read the file content into the text buffer
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        line[strcspn(line, "\n")] = '\0';

        // Resize buffer if needed
        if (text_buffer.num_lines >= text_buffer.capacity) {
            text_buffer.capacity *= 2;
            text_buffer.lines = realloc(text_buffer.lines, sizeof(char*) * text_buffer.capacity);
        }

        text_buffer.lines[text_buffer.num_lines++] = strdup(line);
    }

    // Call render_text_buffer to display the initial content
    render_text_buffer(window, &text_buffer, 0, 0, 0);

    // Enable cursor and allow editing
    curs_set(1);
    keypad(window, TRUE);  // Enable keypad mode to handle special keys
    wmove(window, 1, 2);

    // Initialize cursor position and start line for scrolling
    int cursor_line = 0;
    int cursor_col = 0;
    int start_line = 0;

    bool exit_edit_mode = false;
    while (!exit_edit_mode && (ch = wgetch(window)) != KEY_F(1)) {
        int max_y, max_x;
        getmaxyx(window, max_y, max_x);
        (void)max_x;  // Suppress unused variable warning

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
                    if (cursor_line < start_line) {
                        start_line = cursor_line;
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
            case '\n':
            {
                // Split the current line at the cursor position
                char *current_line = text_buffer.lines[cursor_line];
                char *new_line = strdup(current_line + cursor_col);
                current_line[cursor_col] = '\0';

                // Insert the new line into the buffer
                if (text_buffer.num_lines >= text_buffer.capacity) {
                    text_buffer.capacity *= 2;
                    text_buffer.lines = realloc(text_buffer.lines, sizeof(char*) * text_buffer.capacity);
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
                    // Remove character before cursor
                    char *current_line = text_buffer.lines[cursor_line];
                    memmove(&current_line[cursor_col - 1], &current_line[cursor_col], strlen(current_line) - cursor_col + 1);
                    cursor_col--;
                } else if (cursor_line > 0) {
                    // Merge with previous line
                    int prev_len = (int)strlen(text_buffer.lines[cursor_line - 1]);
                    int curr_len = (int)strlen(text_buffer.lines[cursor_line]);

                    text_buffer.lines[cursor_line - 1] = realloc(text_buffer.lines[cursor_line - 1], prev_len + curr_len + 1);
                    strcat(text_buffer.lines[cursor_line - 1], text_buffer.lines[cursor_line]);
                    free(text_buffer.lines[cursor_line]);

                    // Shift lines up
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
            {
                // Close the current file stream
                fclose(file);

                // Reopen the file in write mode
                file = fopen(file_path, "w");
                if (file == NULL) {
                    mvwprintw(notifwin, max_y - 2, 2, "Error opening file for writing");
                    wrefresh(notifwin);
                    break;
                }

                // Write the content from the text buffer to the file
                for (int i = 0; i < text_buffer.num_lines; i++) {
                    if (fprintf(file, "%s\n", text_buffer.lines[i]) < 0) {
                        mvwprintw(notifwin, max_y - 2, 2, "Error writing to file");
                        wrefresh(notifwin);
                        break;
                    }
                }

                fflush(file);  // Ensure all data is written

                // Close and reopen the file in read/write mode for further operations
                fclose(file);
                file = fopen(file_path, "r+");
                if (file == NULL) {
                    mvwprintw(notifwin, max_y - 2, 2, "Error reopening file");
                    wrefresh(notifwin);
                    break;
                }

                werase(notifwin);
                mvwprintw(notifwin, 0, 0, "File saved: %s", file_path);
                wrefresh(notifwin);
                break;
            }
            case 5:   // Ctrl+E
                exit_edit_mode = true;
                break;
            default:
                if (ch >= 32 && ch <= 126) {
                    // Insert character at cursor position
                    char *current_line = text_buffer.lines[cursor_line];
                    int line_len = (int)strlen(current_line);
                    current_line = realloc(current_line, line_len + 2); // +1 for new char, +1 for null terminator
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

    // Clean up
    for (int i = 0; i < text_buffer.num_lines; i++) {
        free(text_buffer.lines[i]);
    }
    free(text_buffer.lines);
}

/**
 * Checks if the given file has a supported extension.
 *
 * @param filename The name of the file to check.
 * @return true if the file has a supported extension, false otherwise.
 */
bool is_supported_file_type(const char *filename) {
    const char *extensions[] = {
        ".txt", ".c", ".java", ".py", ".cpp", ".h", ".hpp", ".js", ".html", ".css",
        ".md", ".json", ".xml", ".sh", ".bat", ".ini", ".conf", ".log", ".csv", ".tsv",
        ".yaml", ".yml", ".toml", ".rb", ".php", ".pl", ".rs", ".go", ".swift", ".kt"
    };
    size_t num_extensions = sizeof(extensions) / sizeof(extensions[0]);
    const char *ext = strrchr(filename, '.');
    if (ext) {
        for (size_t i = 0; i < num_extensions; i++) {
            if (strcmp(ext, extensions[i]) == 0) {
                return true;
            }
        }
    }
    return false;
}