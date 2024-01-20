#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <utils.h>
#include <vector.h>
#include <files.h>

#define MAX_PATH_LENGTH 256


typedef struct {
    SIZE start;
    SIZE cursor;
    SIZE num_lines;
    SIZE num_files;
} CursorAndSlice;


bool is_hidden(const char *filename) {
    return filename[0] == '.' && (strlen(filename) == 1 || (filename[1] != '.' && filename[1] != '\0'));
}

void draw_directory_window(
    WINDOW *window,
    const char *directory,
    FileAttr *files,
    SIZE files_len,
    SIZE selected_entry
) {
    [[maybe_unused]]
    int cols, lines;
    // No & are needed since this is a macro. Source: X/Open Curses, Issue 4,
    // Version 2. COLS and LINES seem to refer to the terminal size, not the
    // window's.
    getmaxyx(window, lines, cols);

    werase(window);
    // Draw a border around the window
    box(window, 0, 0);
    // Display the directory path
    mvwprintw(window, 1, 1, "Directory: %.*s", cols - 4, directory);

    // Additional logic to fix the displayed directory path
    char corrected_directory[MAX_PATH_LENGTH];
    if (directory[0] != '/') {
        snprintf(corrected_directory, MAX_PATH_LENGTH, "/%s", directory);
        mvwprintw(window, 1, 1, "Directory: %.*s", cols - 4, corrected_directory);
    }

    // Display files in the directory within the visible range
    for (SIZE i = 0; i < files_len; i++) {
        const char *current_name = FileAttr_get_name(files[i]);
        const char *extension = strrchr(current_name, '.');
        int extension_len = extension ? strlen(extension) : 0;

        // Truncate file names that exceed the window width
        int max_display_length = cols - 4;

        if (i == selected_entry)
            wattron(window, A_REVERSE);
        if (FileAttr_is_dir(files[i]))
            wattron(window, A_BOLD);

        if ((int)strlen(current_name) > max_display_length) {
            if (extension_len && extension_len + 5 < max_display_length)
                mvwprintw(
                    window, i + 4, 2,
                    "%.*s... %s",
                    max_display_length - 4 - extension_len, current_name,
                    extension
                );
            else
                mvwprintw(
                    window, i + 4, 2,
                    "%.*s...",
                    max_display_length - 3, current_name
                );
        } else {
            mvwprintw(window, i + 4, 2, "%s", current_name);
        }

        if (i == selected_entry)
            wattroff(window, A_REVERSE);
        if (FileAttr_is_dir(files[i]))
            wattroff(window, A_BOLD);
    }

    // Refresh the window
    wrefresh(window);
}

void draw_preview_window(WINDOW *window, const char *filename, const char *content) {
    // Clear the window
    werase(window);

    // Draw a border around the window
    box(window, 0, 0);

    // Display the filename
    mvwprintw(window, 1, 1, "Previewing file: %.*s", COLS - 4, filename);

    // Display the content of the file
    mvwprintw(window, 3, 1, "Content:\n%.*s", LINES - 6, content);

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
    if (base[strlen(base) - 1] == '/' && extra[0] == '/') {
        // Avoid double slash by skipping the first character of 'extra'
        snprintf(result, MAX_PATH_LENGTH, "%s%s", base, extra + 1);
    } else {
        snprintf(result, MAX_PATH_LENGTH, "%s/%s", base, extra);
    }
}

void reload_directory(Vector *files, const char *current_directory) {
    // Empties the vector
    Vector_set_len(files, 0);
    // Reads the filenames
    append_files_to_vec(files, current_directory);
    // Makes the vector shorter
    Vector_sane_cap(files);
}

void navigate_up(CursorAndSlice *cas) {
    cas->cursor -= 1;
    fix_cursor(cas);
}

void navigate_down(CursorAndSlice *cas) {
    cas->cursor += 1;
    fix_cursor(cas);
}

void navigate_left(char **current_directory, const char *parent_directory, Vector *files) {
    // Check if the current directory is the root directory
    if (strcmp(*current_directory, parent_directory) != 0) {
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
        strcpy(*current_directory, parent_directory);
        reload_directory(files, *current_directory);
    }
}

void navigate_right(char **current_directory, const char *selected_entry, Vector *files, CursorAndSlice *dir_window_cas) {
    char new_path[MAX_PATH_LENGTH];
    path_join(new_path, *current_directory, selected_entry);

    // Save the current cursor position
    SIZE saved_cursor = dir_window_cas->cursor;

    // Check if the selected entry is a directory
    struct stat entry_stat;
    if (stat(new_path, &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
        free(*current_directory);
        *current_directory = strdup(new_path);
        reload_directory(files, *current_directory);

        // Restore the cursor position
        dir_window_cas->cursor = saved_cursor;
    } else {
        // Display a message only if it's not a directory
        mvprintw(LINES - 1, 1, "Selected entry is a file or not a valid directory");
    }
}

// TODO: make it adapt itself when the screen gets resized
int main() {
    WINDOW *mainwin;
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    // Hides the cursor
    // X/Open Curses, Issue 4, Version 2
    curs_set(0);

    // getch() or other read operations, instead of blocking until a key is
    // pressed, block for at most 100 milliseconds. This is useful since it
    // doesn't completely freeze the screen if no keys are typed.
    // This behaviour is described by both X/Open Curses, Issue 4, Version 2
    // and X/Open Curses, Issue 7.
    timeout(100);


    // Create main window
    mainwin = newwin(LINES, COLS, 0, 0);

    wtimeout(mainwin, 100);  // Adjust the timeout value as needed

    // Calculate dimensions for the directory and preview windows
    SIZE dir_win_width = COLS / 2;
    SIZE preview_win_width = COLS - dir_win_width;

    // Create directory window
    WINDOW *dirwin = subwin(mainwin, LINES, dir_win_width, 0, 0);
    box(dirwin, 0, 0);
    wrefresh(dirwin);

    // Create preview window
    WINDOW *previewwin = subwin(mainwin, LINES, preview_win_width, 0, dir_win_width);
    box(previewwin, 0, 0);
    wrefresh(previewwin);

    // Get the default root directory ("/") or user's home directory
    const char *default_directory = getenv("HOME");
    if (default_directory == NULL)
        default_directory = "/";

    char *current_directory = strdup(default_directory);

    Vector files = Vector_new(10);
    append_files_to_vec(&files, current_directory);


    // Dummy filename and content for demonstration
    const char *filename = "";  // You can set a default filename if needed
    const char *content = "This is a placeholder content.";  // You can set default content if needed


    CursorAndSlice dir_window_cas = {
        // Previously called start_entry_dir
        .start = 0,
        // Previously called selected_entry_dir
        .cursor = 0,
        // What used to be end_entry_dir was LINES - 6, and it represented the
        // last valid entry. Therefore the length is LINES - 6 + 1 - start
        .num_lines = LINES - 5,
        // Used for cursor validation
        .num_files = Vector_len(files),
    };

    CursorAndSlice preview_window_cas = {
        // Previously called start_entry_preview
        .start = 0,
        // Previously called selected_entry_preview
        .cursor = 0,
        // What used to be end_entry_preview was LINES - 6, and it represented
        // the last valid entry. Therefore the length is LINES - 6 + 1 - start
        .num_lines = LINES - 5,
        // FIXME: I don't think it should be validated by the number of files
        //        since it isn't the dir window
        .num_files = Vector_len(files),
    };

    enum {
        DIRECTORY_WIN_ACTIVE = 1,
        PREVIEW_WIN_ACTIVE = 2,
    } active_window = DIRECTORY_WIN_ACTIVE;

    int ch;
    while ((ch = getch()) != KEY_F(1)) {
        // Handle key presses and update screen

        // ERR is returned if nothing has been pressed for 100ms
        if (ch != ERR) {
            switch (ch) {
                case KEY_UP:
                    // Move up in the active window
                    if (active_window == DIRECTORY_WIN_ACTIVE)
                        navigate_up(&dir_window_cas);
                    else
                        navigate_up(&preview_window_cas);
                    break;
                case KEY_DOWN:
                    // Move down in the active window
                    if (active_window == DIRECTORY_WIN_ACTIVE)
                        navigate_down(&dir_window_cas);
                    else
                        navigate_down(&preview_window_cas);
                    break;
                case KEY_LEFT:
                    // Navigate left (go up in the directory tree)
                    navigate_left(&current_directory, "/", &files);
                    // Reload the file list for the new directory
                    // (similar code as initializing the file list)
                    // ...

                    // Reset selected entries and scroll positions
                    dir_window_cas.cursor = preview_window_cas.cursor = 0;
                    dir_window_cas.start = preview_window_cas.start = 0;
                    dir_window_cas.num_lines = preview_window_cas.num_lines = LINES - 5;
                    dir_window_cas.num_files = preview_window_cas.num_files = Vector_len(files);
                    break;
                case KEY_RIGHT:
                    // Navigate right (go into the selected directory)
                    navigate_right(
                            &current_directory,
                            FileAttr_get_name(files.el[dir_window_cas.cursor]),
                            &files,
                            &dir_window_cas
                    );

                    // Reload the file list for the new directory
                    // (similar code as initializing the file list)
                    // ...

                    // FIXME: repeated code
                    dir_window_cas.cursor = preview_window_cas.cursor = 0;
                    dir_window_cas.start = preview_window_cas.start = 0;
                    dir_window_cas.num_lines = preview_window_cas.num_lines = LINES - 5;
                    dir_window_cas.num_files = preview_window_cas.num_files = Vector_len(files);
                    break;
                default:
                    // Print the key code for debugging purposes
                    mvwprintw(mainwin, LINES - 1, 1, "Key pressed: %d", ch);
                    break;
            }
        }

        // Draw the directory window
        draw_directory_window(
            dirwin, current_directory,
            (FileAttr *)&files.el[dir_window_cas.start],
            // TODO: make sure that its impossible for num_lines to get past
            //       num_files.
            MIN(dir_window_cas.num_lines, dir_window_cas.num_files - dir_window_cas.start),
            dir_window_cas.cursor - dir_window_cas.start
        );

        // Draw the preview window
        draw_preview_window(previewwin, filename, content);

        // Refresh the main window
        wrefresh(mainwin);
    }


    Vector_bye(&files);
    free(current_directory);

    // Clean up
    endwin();
    return 0;
}
