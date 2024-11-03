// File: main.c
// -----------------------
#include <stdio.h>     // for snprintf
#include <stdlib.h>    // for free, malloc
#include <unistd.h>    // for getenv
#include <curses.h>    // for initscr, noecho, cbreak, keypad, curs_set, timeout, endwin, LINES, COLS, getch, timeout, wtimeout, ERR, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_F1, newwin, subwin, box, wrefresh, werase, mvwprintw, wattron, wattroff, A_REVERSE, A_BOLD, getmaxyx, refresh
#include <dirent.h>    // for opendir, readdir, closedir
#include <sys/types.h> // for types like SIZE
#include <sys/stat.h>  // for struct stat
#include <string.h>    // for strlen, strcpy, strdup, strrchr, strtok, strncmp
#include <signal.h>    // for signal, SIGWINCH
// Local includes
#include <utils.h>     // for MIN, MAX
#include <vector.h>    // for Vector
#include <files.h>     // for FileAttr, FileAttr_get_name, FileAttr_is_dir
#include <vecstack.h>  // for VecStack, VecStack_empty, VecStack_push, VecStack_pop
#include <files.h>     // for path_join, is_supported_file_type, display_file_info
#include <utils.h>     // for die

#define MAX_PATH_LENGTH 256
#define TAB 9
#define CTRL_E 5

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

void draw_preview_window(WINDOW *window, const char *current_directory, const char *selected_entry) {
    // Clear the window
    werase(window);

    // Draw a border around the window
    box(window, 0, 0);

    // Display the selected entry path
    char file_path[MAX_PATH_LENGTH];
    path_join(file_path, current_directory, selected_entry);
    mvwprintw(window, 0, 2, "Selected Entry: %.*s", COLS - 4, file_path);

    // Get the window's dimensions
    int max_x, max_y;
    getmaxyx(window, max_y, max_x);

    // Display file info
    display_file_info(window, file_path, max_x);

    // Check if the selected entry is a supported file type
    if (is_supported_file_type(file_path)) {
        FILE *file = fopen(file_path, "r");
        if (file) {
            char line[256];
            int line_num = 5; // Start displaying file content from line 5

            // Add a blank line before the file content
            mvwprintw(window, line_num++, 2, " ");

            // add a line, mentioning "Previewing file: <filename>"
			            mvwprintw(window, line_num++, 2, "Previewing file: %s", selected_entry);
			while (fgets(line, sizeof(line), file) && line_num < max_y - 2) {
                mvwprintw(window, line_num++, 2, "%.*s", max_x - 2, line);
            }

            // Add a blank line after the file content
            mvwprintw(window, line_num++, 2, " ");

            fclose(file);
        } else {
            mvwprintw(window, 5, 2, "Unable to open file for preview");
        }
    }

    // Refresh the window
    wrefresh(window);
}
void fix_cursor(CursorAndSlice *cas) {
    cas->cursor = MIN(cas->cursor, cas->num_files - 1);
    cas->cursor = MAX(0, cas->cursor);

    cas->start = MIN(cas->start, cas->cursor);
    cas->start = MAX(cas->start, cas->cursor + 1 - cas->num_lines);
}

void path_join(char *result, const char *base, const char *extra) {
    size_t base_len = strlen(base);
    size_t extra_len = strlen(extra);

    if (base_len == 0) {
        // If the base is an empty string, copy the extra to the result
        strncpy(result, extra, MAX_PATH_LENGTH);
    } else if (extra_len == 0) {
        // If the extra is an empty string, copy the base to the result
        strncpy(result, base, MAX_PATH_LENGTH);
    } else {
        // Check if the base ends with a slash
        if (base[base_len - 1] == '/') {
            // No need to skip the first character of 'extra'
            snprintf(result, MAX_PATH_LENGTH, "%s%s", base, extra);
        } else {
            snprintf(result, MAX_PATH_LENGTH, "%s/%s", base, extra);
        }
    }

    // Ensure null-terminated
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
    cas->cursor -= 1;
    fix_cursor(cas);
    *selected_entry = FileAttr_get_name(files->el[cas->cursor]);
}

void navigate_down(CursorAndSlice *cas, const Vector *files, const char **selected_entry) {
    cas->cursor += 1;
    fix_cursor(cas);
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
void navigate_right(char **current_directory, const char *selected_entry, Vector *files, CursorAndSlice *dir_window_cas) {
    // Check if the selected entry is a directory
    if (!FileAttr_is_dir(files->el[dir_window_cas->cursor])) {
        // If not a directory, simply return
        return;
    }

    char new_path[MAX_PATH_LENGTH];
    path_join(new_path, *current_directory, selected_entry);

    // Save the current cursor position
    SIZE saved_cursor = dir_window_cas->cursor;

    // Push the selected entry onto the stack
    char* new_entry = strdup(selected_entry);
    if (new_entry == NULL) {
        mvprintw(LINES - 1, 1, "Memory allocation error");
        refresh();
        return;
    }
    VecStack_push(&directoryStack, new_entry);

    // Update the current directory and reload files
    free(*current_directory);
    *current_directory = strdup(new_path);
    if (*current_directory == NULL) {
        mvprintw(LINES - 1, 1, "Memory allocation error");
        refresh();
        VecStack_pop(&directoryStack);  // Rollback stack operation
        return;
    }

    // Check if the new directory is empty
    if (Vector_len(*files) == 0) {
        mvprintw(LINES - 1, 1, "Empty directory");
        refresh();
        VecStack_pop(&directoryStack);  // Rollback stack operation
        return;
    }

    reload_directory(files, *current_directory);

    refresh();

    // Restore the cursor position based on the new directory's contents
    dir_window_cas->cursor = saved_cursor;
    dir_window_cas->start = 0; // Reset other parameters if needed
    dir_window_cas->num_lines = LINES - 5;
    dir_window_cas->num_files = Vector_len(*files);
}

void handle_winch(int sig) {
    (void)sig;  // Suppress unused parameter warning

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
// todo: make a bottom win to show error messages, current commands, and keybinds being used
// TODO: fix when resize the files go voer the border
int main() {
    initscr();  // Initialize the screen
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    // Initialize notifwin after initscr()
    notifwin = newwin(1, COLS, LINES - 1, 0);
    // Initialize notifwin
    werase(notifwin);
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

    signal(SIGWINCH, handle_winch);  // Handle window resizing

    directoryStack = VecStack_empty();

    char *current_directory = malloc(MAX_PATH_LENGTH);
    if (current_directory == NULL) {
        die(1, "Memory allocation error");
    }

    if (getcwd(current_directory, MAX_PATH_LENGTH) == NULL) {
        die(1, "Unable to get current working directory");
    }

    const char *selected_entry = "";

    Vector files = Vector_new(10);
    append_files_to_vec(&files, current_directory);

    CursorAndSlice dir_window_cas = {
        .start = 0,
        .cursor = 0,
        .num_lines = LINES - 5,
        .num_files = Vector_len(files),
    };

    enum {
        DIRECTORY_WIN_ACTIVE = 1,
        PREVIEW_WIN_ACTIVE = 2,
    } active_window = DIRECTORY_WIN_ACTIVE;

    int ch;
    while ((ch = getch()) != KEY_F(1)) {
        bool should_clear_notif = true;
        if (ch != ERR) {
            switch (ch) {
            case KEY_UP:
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    navigate_up(&dir_window_cas, &files, &selected_entry);
                    werase(notifwin);
                    mvwprintw(notifwin, 0, 0, "Moved up");
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
                break;
            case KEY_DOWN:
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    navigate_down(&dir_window_cas, &files, &selected_entry);
                    // Update notifwin
                    werase(notifwin);
                    mvwprintw(notifwin, 0, 0, "Moved down");
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
                    break;
            case KEY_LEFT:
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    navigate_left(&current_directory, &files, &dir_window_cas);
                    // Update notifwin
                    werase(notifwin);
                    mvwprintw(notifwin, 0, 0, "Navigated to parent directory");
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
                break;
            case KEY_RIGHT:
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    navigate_right(&current_directory, selected_entry, &files, &dir_window_cas);
                    // Update notifwin
                    werase(notifwin);
                    mvwprintw(notifwin, 0, 0, "Entered directory: %s", selected_entry);
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
                break;
            case TAB:
                active_window = (active_window == DIRECTORY_WIN_ACTIVE) ? PREVIEW_WIN_ACTIVE : DIRECTORY_WIN_ACTIVE;
                // Update notifwin
                werase(notifwin);
                mvwprintw(notifwin, 0, 0, "Switched to %s window", (active_window == DIRECTORY_WIN_ACTIVE) ? "Directory" : "Preview");
                wrefresh(notifwin);
                should_clear_notif = false;
                break;
            case CTRL_E:
                if (active_window == PREVIEW_WIN_ACTIVE) {
                    char file_path[MAX_PATH_LENGTH];
                    path_join(file_path, current_directory, selected_entry);
                    edit_file_in_terminal(previewwin, file_path, notifwin);
                    // Update notifwin
                    werase(notifwin);
                    mvwprintw(notifwin, 0, 0, "Editing file: %s", selected_entry);
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

        // Refresh the windows
        draw_directory_window(dirwin, current_directory, (FileAttr *)&files.el[dir_window_cas.start], MIN(dir_window_cas.num_lines, dir_window_cas.num_files - dir_window_cas.start), dir_window_cas.cursor - dir_window_cas.start);
        draw_preview_window(previewwin, current_directory, selected_entry);

        // Highlight the active window
        if (active_window == DIRECTORY_WIN_ACTIVE) {
            wattron(dirwin, A_REVERSE);
            mvwprintw(dirwin, dir_window_cas.cursor - dir_window_cas.start + 1, 1, "%s", FileAttr_get_name(files.el[dir_window_cas.cursor]));
            wattroff(dirwin, A_REVERSE);
        } else {
            wattron(previewwin, A_REVERSE);
            mvwprintw(previewwin, 1, 1, "Preview Window Active");
            wattroff(previewwin, A_REVERSE);
        }

        wrefresh(mainwin);
        // Note: Ensure you refresh the notification window after other windows
        wrefresh(notifwin);
    }

    // Clean up
    Vector_bye(&files);
    free(current_directory);
    delwin(dirwin);
    delwin(previewwin);
    delwin(notifwin);
    delwin(mainwin);
    endwin();
    return 0;
}