// File: main.c
// -----------------------
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>     // for snprintf
#include <stdlib.h>    // for free, malloc
#include <unistd.h>    // for getenv
#include <curses.h>    // for initscr, noecho, cbreak, keypad, curs_set, timeout, endwin, LINES, COLS, getch, timeout, wtimeout, ERR, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_F1, newwin, subwin, box, wrefresh, werase, mvwprintw, wattron, wattroff, A_REVERSE, A_BOLD, getmaxyx, refresh
#include <dirent.h>    // for opendir, readdir, closedir
#include <sys/types.h> // for types like SIZE
#include <sys/stat.h>  // for struct stat
#include <string.h>    // for strlen, strcpy, strdup, strrchr, strtok, strncmp
#include <signal.h>    // for signal, SIGWINCH
#include <stdbool.h>   // for bool, true, false
#include <string.h> // For memset
#include <magic.h> // For libmagic
#include <time.h> // For strftime
// Local includes
#include "utils.h"
#include "vector.h"
#include <files.h>
#include "vecstack.h"

#define MAX_PATH_LENGTH 256 // 256
#define TAB 9
#define CTRL_E 5

// Global resize flag
volatile sig_atomic_t resized = 0;

// Global variables
WINDOW *notifwin;
WINDOW *mainwin;
WINDOW *dirwin;
WINDOW *previewwin;

VecStack directoryStack;

typedef struct {
    SIZE start;
    SIZE cursor;
    SIZE num_lines;
    SIZE num_files;
} CursorAndSlice;

typedef struct {
    char *current_directory;
    Vector files;
    CursorAndSlice dir_window_cas;
    const char *selected_entry;
    int preview_start_line;
    // Add more state variables here if needed
} AppState;

// Recursive function to display directory tree structure
void show_directory_tree(WINDOW *window, const char *dir_path, int level, int *line_num, int max_y, int max_x) {
    // Check if the current line number exceeds the window's visible area
    if (*line_num >= max_y - 1) {
        mvwprintw(window, *line_num, 2, "...");
        return;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    struct stat statbuf;
    char full_path[MAX_PATH_LENGTH];
    char truncated_path[MAX_PATH_LENGTH];
    bool truncated = false; // Flag to indicate path truncation

    // Iterate through directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        // Construct full path
        int result = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (result < 0 || result >= (int)sizeof(full_path)) {
            truncated = true;
            // Use a separate buffer for the truncated path
            snprintf(truncated_path, sizeof(truncated_path), "%.*s...", MAX_PATH_LENGTH - 4, full_path);
        } else {
            truncated = false;
            strncpy(truncated_path, full_path, sizeof(truncated_path) - 1);
            truncated_path[sizeof(truncated_path) - 1] = '\0';
        }

        // Retrieve file stats
        if (lstat(full_path, &statbuf) == -1) continue;

        // Display the entry name with indentation and truncation indicator
        if (*line_num < max_y - 1) {
            const char *display_name = truncated ? "[Path too long]" : entry->d_name;
            mvwprintw(window, (*line_num)++, 2 + level * 2, "%.*s", max_x - 4 - level * 2, display_name);

            // Display additional directory information
            if (S_ISDIR(statbuf.st_mode)) {
                wattron(window, A_BOLD); // Highlight directories
                mvwprintw(window, *line_num - 1, 2 + level * 2 + strlen(display_name), " [DIR]");
                wattroff(window, A_BOLD);
            }

            // Optionally add more details (size, permissions, etc.)
            char perm[10];
            snprintf(perm, sizeof(perm), "%o", statbuf.st_mode & 0777);
            mvwprintw(window, *line_num - 1, max_x - 10, "%s", perm); // Permissions display
        }

        // Recursively list directories
        if (S_ISDIR(statbuf.st_mode)) {
            show_directory_tree(window, truncated_path, level + 1, line_num, max_y, max_x);
        }
    }
    closedir(dir);
}

// Function to update the stack when navigating left or right
void updateDirectoryStack(const char *newDirectory) {
    char *token;
    char *copy = strdup(newDirectory);

    // Push each directory onto the stack
    for (token = strtok(copy, "/"); token; token = strtok(NULL, "/")) {
        VecStack_push(&directoryStack, strdup(token));
    }

    free(copy);
}

bool is_hidden(const char *filename) {
    return filename[0] == '.' && (strlen(filename) == 1 || (filename[1] != '.' && filename[1] != '\0'));
}

// Helper function to count total lines in a file
int get_total_lines(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) return 0;

    int total_lines = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        total_lines++;
    }

    fclose(file);
    return total_lines;
}
// tab / clicking on the different windows will move the cursor to that window, will be used later for editing files
void draw_directory_window(
        WINDOW *window,
        const char *directory,
        FileAttr *files,
        SIZE files_len,
        SIZE selected_entry
) {
    [[maybe_unused]]
    int cols, lines;
    getmaxyx(window, lines, cols);

    werase(window);
    box(window, 0, 0);
    mvwprintw(window, 0, 2, "Directory: %.*s", cols - 4, directory);

    for (SIZE i = 0; i < files_len; i++) {
        const char *current_name = FileAttr_get_name(files[i]);
        const char *extension = strrchr(current_name, '.');
        int extension_len = extension ? strlen(extension) : 0;

        int max_display_length = cols - 4;

        if (i == selected_entry)
            wattron(window, A_REVERSE);
        if (FileAttr_is_dir(files[i]))
            wattron(window, A_BOLD);

        if ((int)strlen(current_name) > max_display_length) {
            if (extension_len && extension_len + 5 < max_display_length)
                mvwprintw(
                        window, i + 2, 2,
                        "%.*s... %s",
                        max_display_length - 4 - extension_len, current_name,
                        extension
                );
            else
                mvwprintw(
                        window, i + 2, 2,
                        "%.*s...",
                        max_display_length - 3, current_name
                );
        } else {
            mvwprintw(window, i + 2, 2, "%s", current_name);
        }

        if (i == selected_entry)
            wattroff(window, A_REVERSE);
        if (FileAttr_is_dir(files[i]))
            wattroff(window, A_BOLD);
    }

    wrefresh(window);
}

void draw_preview_window(WINDOW *window, const char *current_directory, const char *selected_entry, int start_line) {
    // Clear the window and draw a border
    werase(window);
    box(window, 0, 0);

    // Get window dimensions
    int max_x, max_y;
    getmaxyx(window, max_y, max_x);

    // Display the selected entry path
    char file_path[MAX_PATH_LENGTH];
    path_join(file_path, current_directory, selected_entry);
    mvwprintw(window, 0, 2, "Selected Entry: %.*s", max_x - 4, file_path);

    // Attempt to retrieve file information
    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        mvwprintw(window, 2, 2, "Unable to retrieve file information");
        return;
    }

    // Display file size
    char fileSizeStr[20];
    format_file_size(fileSizeStr, file_stat.st_size);
    mvwprintw(window, 2, 2, "File Size: %s", fileSizeStr);

    // Display file permissions
    char permissions[10];
    snprintf(permissions, sizeof(permissions), "%o", file_stat.st_mode & 0777);
    mvwprintw(window, 3, 2, "Permissions: %s", permissions);

    // Display last modification time
    char modTime[50];
    strftime(modTime, sizeof(modTime), "%c", localtime(&file_stat.st_mtime));
    mvwprintw(window, 4, 2, "Last Modified: %s", modTime);

    // Display MIME type using libmagic
    magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
    if (magic_cookie != NULL && magic_load(magic_cookie, NULL) == 0) {
        const char *mime_type = magic_file(magic_cookie, file_path);
        mvwprintw(window, 5, 2, "MIME Type: %s", mime_type ? mime_type : "Unknown");
        magic_close(magic_cookie);
    } else {
        mvwprintw(window, 5, 2, "MIME Type: Unable to detect");
    }

    // If the file is a directory, display the directory contents
    if (S_ISDIR(file_stat.st_mode)) {
        int line_num = 7;
        show_directory_tree(window, file_path, 0, &line_num, max_y, max_x);
    } else if (is_supported_file_type(file_path)) {
        // Display file preview for supported types
        FILE *file = fopen(file_path, "r");
        if (file) {
            char line[256];
            int line_num = 7;
            int current_line = 0;

            // Skip lines until start_line
            while (current_line < start_line && fgets(line, sizeof(line), file)) {
                current_line++;
            }

            // Display file content from start_line onward
            while (fgets(line, sizeof(line), file) && line_num < max_y - 1) {
                line[strcspn(line, "\n")] = '\0'; // Remove newline character
                mvwprintw(window, line_num++, 2, "%.*s", max_x - 4, line);
            }

            fclose(file);

            if (line_num < max_y - 1) {
                mvwprintw(window, line_num++, 2, "End of file");
            }
        } else {
            mvwprintw(window, 7, 2, "Unable to open file for preview");
        }
    }

    // Refresh to show changes
    wrefresh(window);
}

void redraw_all_windows(AppState *state) {
    // Update ncurses internal structures to reflect the new terminal size
    endwin();
    refresh();
    clear();

    // Recalculate window dimensions based on new LINES and COLS
    int new_cols = COLS;
    int new_lines = LINES;

    // Resize main window
    wresize(mainwin, new_lines - 1, new_cols);
    mvwin(mainwin, 0, 0);
    werase(mainwin);
    box(mainwin, 0, 0);
    wrefresh(mainwin);

    // Resize notifwin
    wresize(notifwin, 1, new_cols);
    mvwin(notifwin, new_lines - 1, 0);
    werase(notifwin);
    box(notifwin, 0, 0);
    wrefresh(notifwin);

    // Calculate new widths
    SIZE dir_win_width = new_cols / 2;
    SIZE preview_win_width = new_cols - dir_win_width;

    // Resize and move dirwin
    wresize(dirwin, new_lines - 1, dir_win_width);
    mvwin(dirwin, 0, 0);
    werase(dirwin);
    box(dirwin, 0, 0);
    wrefresh(dirwin);

    // Resize and move previewwin
    wresize(previewwin, new_lines - 1, preview_win_width);
    mvwin(previewwin, 0, dir_win_width);
    werase(previewwin);
    box(previewwin, 0, 0);
    wrefresh(previewwin);

    // Redraw directory and preview contents
    draw_directory_window(
            dirwin,
            state->current_directory,
            (FileAttr *)&state->files.el[state->dir_window_cas.start],
            MIN(state->dir_window_cas.num_lines, state->dir_window_cas.num_files - state->dir_window_cas.start),
            state->dir_window_cas.cursor - state->dir_window_cas.start
    );

    draw_preview_window(
            previewwin,
            state->current_directory,
            state->selected_entry,
            state->preview_start_line
    );
}

void fix_cursor(CursorAndSlice *cas) {
    cas->cursor = MIN(cas->cursor, cas->num_files - 1);
    cas->cursor = MAX(0, cas->cursor);

    cas->start = MIN(cas->start, cas->cursor);
    cas->start = MAX(cas->start, cas->cursor + 1 - cas->num_lines);
}

// Remove this entire function from main.c
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

void reload_directory(Vector *files, const char *current_directory) {
    // Empties the vector
    Vector_set_len(files, 0);
    // Reads the filenames
    append_files_to_vec(files, current_directory);
    // Makes the vector shorter
    Vector_sane_cap(files);
}

void navigate_up(CursorAndSlice *cas, const Vector *files, const char **selected_entry) {
    if (cas->num_files > 1) {
        cas->cursor -= 1;
        fix_cursor(cas);
    }
    *selected_entry = FileAttr_get_name(files->el[cas->cursor]);
}

void navigate_down(CursorAndSlice *cas, const Vector *files, const char **selected_entry) {
    if (cas->num_files > 1) {
        cas->cursor += 1;
        fix_cursor(cas);
    }
    *selected_entry = FileAttr_get_name(files->el[cas->cursor]);
}

void navigate_left(char **current_directory, Vector *files, CursorAndSlice *dir_window_cas) {
    // Check if the current directory is the root directory
    if (strcmp(*current_directory, "/") != 0) {
        // If not the root directory, move up one level
        char *last_slash = strrchr(*current_directory, '/');
        if (last_slash != NULL) {
            *last_slash = '\0'; // Remove the last directory from the path
            reload_directory(files, *current_directory);
        }
    }

    // Check if the current directory is now an empty string
    if ((*current_directory)[0] == '\0') {
        // If empty, set it back to the root directory
        strcpy(*current_directory, "/");
        reload_directory(files, *current_directory);
    }

    // Pop the last directory from the stack
    free(VecStack_pop(&directoryStack));

    // Reset selected entries and scroll positions
    dir_window_cas->cursor = 0;
    dir_window_cas->start = 0;
    dir_window_cas->num_lines = LINES - 5;
    dir_window_cas->num_files = Vector_len(*files);
}

// Function to navigate right
void navigate_right(AppState *state, char **current_directory, const char *selected_entry, Vector *files, CursorAndSlice *dir_window_cas) {
    // Verify if the selected entry is a directory
    FileAttr current_file = files->el[dir_window_cas->cursor];
    if (!FileAttr_is_dir(current_file)) {
        werase(notifwin);
        mvwprintw(notifwin, 0, 0, "Selected entry is not a directory");
        wrefresh(notifwin);
        return;
    }

    // Construct the new path carefully
    char new_path[MAX_PATH_LENGTH];
    path_join(new_path, *current_directory, selected_entry);

    // Check if we’re not re-entering the same directory path
    if (strcmp(new_path, *current_directory) == 0) {
        werase(notifwin);
        mvwprintw(notifwin, 0, 0, "Already in this directory");
        wrefresh(notifwin);
        return;
    }

    // Push the selected directory name onto the stack
    char *new_entry = strdup(selected_entry);
    if (new_entry == NULL) {
        mvwprintw(notifwin, LINES - 1, 1, "Memory allocation error");
        wrefresh(notifwin);
        return;
    }
    VecStack_push(&directoryStack, new_entry);

    // Free the old directory and set to the new path
    free(*current_directory);
    *current_directory = strdup(new_path);
    if (*current_directory == NULL) {
        mvwprintw(notifwin, LINES - 1, 1, "Memory allocation error");
        wrefresh(notifwin);
        free(VecStack_pop(&directoryStack));  // Roll back the stack operation
        return;
    }

    // Reload directory contents in the new path
    reload_directory(files, *current_directory);

    // Reset cursor and start position for the new directory
    dir_window_cas->cursor = 0;
    dir_window_cas->start = 0;
    dir_window_cas->num_lines = LINES - 5;
    dir_window_cas->num_files = Vector_len(*files);

    // If there’s only one entry, automatically select it
    if (Vector_len(*files) == 1) {
        state->selected_entry = FileAttr_get_name(files->el[0]);
    }

    wrefresh(mainwin);
}

void handle_winch(int sig) {
    (void)sig;  // Suppress unused parameter warning
    resized = 1;
    // Reinitialize the screen to update LINES and COLS
    endwin();
    refresh();
    clear();

    // Adjust notifwin position and size
    wresize(notifwin, 1, COLS);
    mvwin(notifwin, LINES - 1, 0);
    wclear(notifwin);
    box(notifwin, 0, 0);
    wrefresh(notifwin);

    // Adjust mainwin and its subwindows
    wresize(mainwin, LINES - 1, COLS);
    wclear(mainwin);
    box(mainwin, 0, 0);
    wrefresh(mainwin);

    // Adjust dirwin and previewwin sizes
    SIZE dir_win_width = COLS / 2;
    SIZE preview_win_width = COLS - dir_win_width;

    wresize(dirwin, LINES - 1, dir_win_width);
    wresize(previewwin, LINES - 1, preview_win_width);
    mvwin(previewwin, 0, dir_win_width);

    wclear(dirwin);
    box(dirwin, 0, 0);
    wrefresh(dirwin);

    wclear(previewwin);
    box(previewwin, 0, 0);
    wrefresh(previewwin);
}

// TODO: make it adapt itself when the screen gets resized
// TODO: make a bottom win to show error messages, current commands, and keybinds being used
// TODO: fix when resize the files go over the border
int main() {
    // Initialize ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    // Initialize windows
    notifwin = newwin(1, COLS, LINES - 1, 0);
    werase(notifwin);
    box(notifwin, 0, 0);
    wrefresh(notifwin);

    mainwin = newwin(LINES - 1, COLS, 0, 0);
    wtimeout(mainwin, 100);

    SIZE dir_win_width = COLS / 2;
    SIZE preview_win_width = COLS - dir_win_width;

    dirwin = subwin(mainwin, LINES - 1, dir_win_width, 0, 0);
    box(dirwin, 0, 0);
    wrefresh(dirwin);

    previewwin = subwin(mainwin, LINES - 1, preview_win_width, 0, dir_win_width);
    box(previewwin, 0, 0);
    wrefresh(previewwin);

    // Set up signal handler for window resize
    struct sigaction sa;
    sa.sa_handler = handle_winch;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGWINCH, &sa, NULL);

    // Initialize application state
    AppState state;
    state.current_directory = malloc(MAX_PATH_LENGTH);
    if (state.current_directory == NULL) {
        die(1, "Memory allocation error");
    }

    if (getcwd(state.current_directory, MAX_PATH_LENGTH) == NULL) {
        die(1, "Unable to get current working directory");
    }

    state.selected_entry = "";

    state.files = Vector_new(10);
    append_files_to_vec(&state.files, state.current_directory);

    state.dir_window_cas = (CursorAndSlice){
            .start = 0,
            .cursor = 0,
            .num_lines = LINES - 5,
            .num_files = Vector_len(state.files),
    };

    state.preview_start_line = 0;

    enum {
        DIRECTORY_WIN_ACTIVE = 1,
        PREVIEW_WIN_ACTIVE = 2,
    } active_window = DIRECTORY_WIN_ACTIVE;

    // Initial drawing
    redraw_all_windows(&state);

    int ch;
    while ((ch = getch()) != KEY_F(1)) {
        if (resized) {
            redraw_all_windows(&state);
            resized = 0;
        }

        bool should_clear_notif = true;
        if (ch != ERR) {
            switch (ch) {
                case KEY_UP:
                    if (active_window == DIRECTORY_WIN_ACTIVE) {
                        navigate_up(&state.dir_window_cas, &state.files, &state.selected_entry);
                        state.preview_start_line = 0;
                        werase(notifwin);
                        mvwprintw(notifwin, 0, 0, "Moved up");
                        wrefresh(notifwin);
                        should_clear_notif = false;
                    } else if (active_window == PREVIEW_WIN_ACTIVE) {
                        if (state.preview_start_line > 0) {
                            state.preview_start_line--;
                            werase(notifwin);
                            mvwprintw(notifwin, 0, 0, "Scrolled up");
                            wrefresh(notifwin);
                            should_clear_notif = false;
                        }
                    }
                    break;
                case KEY_DOWN:
                    if (active_window == DIRECTORY_WIN_ACTIVE) {
                        navigate_down(&state.dir_window_cas, &state.files, &state.selected_entry);
                        state.preview_start_line = 0;
                        werase(notifwin);
                        mvwprintw(notifwin, 0, 0, "Moved down");
                        wrefresh(notifwin);
                        should_clear_notif = false;
                    } else if (active_window == PREVIEW_WIN_ACTIVE) {
                        // Determine the total lines in the file
                        char file_path[MAX_PATH_LENGTH];
                        path_join(file_path, state.current_directory, state.selected_entry);
                        int total_lines = get_total_lines(file_path);

                        // Get window dimensions to calculate max_start_line
                        int max_x, max_y;
                        getmaxyx(previewwin, max_y, max_x);
                        (void)max_x;

                        int content_height = max_y - 7;

                        int max_start_line = total_lines - content_height;
                        if (max_start_line < 0) max_start_line = 0;

                        if (state.preview_start_line < max_start_line) {
                            state.preview_start_line++;
                            werase(notifwin);
                            mvwprintw(notifwin, 0, 0, "Scrolled down");
                            wrefresh(notifwin);
                            should_clear_notif = false;
                        }
                    }
                    break;
                case KEY_LEFT:
                    if (active_window == DIRECTORY_WIN_ACTIVE) {
                        navigate_left(&state.current_directory, &state.files, &state.dir_window_cas);
                        state.preview_start_line = 0;
                        werase(notifwin);
                        mvwprintw(notifwin, 0, 0, "Navigated to parent directory");
                        wrefresh(notifwin);
                        should_clear_notif = false;
                    }
                    break;
                    // In the main loop after navigating right
                case KEY_RIGHT:
                    if (active_window == DIRECTORY_WIN_ACTIVE) {
                        if (FileAttr_is_dir(state.files.el[state.dir_window_cas.cursor])) {
                            navigate_right(&state, &state.current_directory, state.selected_entry, &state.files, &state.dir_window_cas);
                            state.preview_start_line = 0;

                            // Automatically set selected_entry if there is only one option
                            if (state.dir_window_cas.num_files == 1) {
                                state.selected_entry = FileAttr_get_name(state.files.el[0]);
                            }

                            werase(notifwin);
                            mvwprintw(notifwin, 0, 0, "Entered directory: %s", state.selected_entry);
                            wrefresh(notifwin);
                            should_clear_notif = false;
                        } else {
                            werase(notifwin);
                            mvwprintw(notifwin, 0, 0, "Selected entry is not a directory");
                            wrefresh(notifwin);
                            should_clear_notif = false;
                        }
                    }
                    break;
                case TAB:
                    active_window = (active_window == DIRECTORY_WIN_ACTIVE) ? PREVIEW_WIN_ACTIVE : DIRECTORY_WIN_ACTIVE;
                    if (active_window == DIRECTORY_WIN_ACTIVE) {
                        state.preview_start_line = 0;
                    }
                    werase(notifwin);
                    mvwprintw(notifwin, 0, 0, "Switched to %s window", (active_window == DIRECTORY_WIN_ACTIVE) ? "Directory" : "Preview");
                    wrefresh(notifwin);
                    should_clear_notif = false;
                    break;
                case CTRL_E:
                    if (active_window == PREVIEW_WIN_ACTIVE) {
                        char file_path[MAX_PATH_LENGTH];
                        path_join(file_path, state.current_directory, state.selected_entry);
                        edit_file_in_terminal(previewwin, file_path, notifwin);
                        state.preview_start_line = 0;
                        werase(notifwin);
                        mvwprintw(notifwin, 0, 0, "Editing file: %s", state.selected_entry);
                        wrefresh(notifwin);
                        should_clear_notif = false;
                    }
                    break;
                default:
                    break;
            }
        }

        // Clear notification window only if no new notification was displayed
        if (should_clear_notif) {
            werase(notifwin);
            wrefresh(notifwin);
        }

        // Redraw windows
        draw_directory_window(
                dirwin,
                state.current_directory,
                (FileAttr *)&state.files.el[state.dir_window_cas.start],
                MIN(state.dir_window_cas.num_lines, state.dir_window_cas.num_files - state.dir_window_cas.start),
                state.dir_window_cas.cursor - state.dir_window_cas.start
        );

        draw_preview_window(
                previewwin,
                state.current_directory,
                state.selected_entry,
                state.preview_start_line
        );

        // Highlight the active window
        if (active_window == DIRECTORY_WIN_ACTIVE) {
            wattron(dirwin, A_REVERSE);
            mvwprintw(dirwin, state.dir_window_cas.cursor - state.dir_window_cas.start + 1, 1, "%s", FileAttr_get_name(state.files.el[state.dir_window_cas.cursor]));
            wattroff(dirwin, A_REVERSE);
        } else {
            wattron(previewwin, A_REVERSE);
            mvwprintw(previewwin, 1, 1, "Preview Window Active");
            wattroff(previewwin, A_REVERSE);
        }

        wrefresh(mainwin);
        wrefresh(notifwin);
    }

    // Clean up
    Vector_bye(&state.files);
    free(state.current_directory);
    delwin(dirwin);
    delwin(previewwin);
    delwin(notifwin);
    delwin(mainwin);
    endwin();
    return 0;
}
