#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <utils.h>
#include <vector.h>

#define MAX_PATH_LENGTH 256
#define MAX_DISPLAY_LENGTH 32


typedef struct {
    SIZE start;
    SIZE cursor;
    SIZE num_lines;
    SIZE num_files;
} CursorAndSlice;



bool is_hidden(const char *filename) {
    return filename[0] == '.' && (strlen(filename) == 1 || (filename[1] != '.' && filename[1] != '\0'));
}

bool is_directory(const char *path, const char *filename) {
    struct stat path_stat;
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);

    if (stat(full_path, &path_stat) == 0)
        return S_ISDIR(path_stat.st_mode);

    return true;
}

void append_files_to_vec(Vector *v, const char *name) {
    DIR *dir = opendir(name);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Filter out "." and ".." entries
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                Vector_add(v, 1);
                v->el[Vector_len(*v)] = strdup(entry->d_name);
                Vector_set_len(v, Vector_len(*v) + 1);
            }
        }
        closedir(dir);
    }
}


void draw_directory_window(
	WINDOW *window,
	const char *directory,
	char **files,
	SIZE files_len,
	SIZE selected_entry
) {
    // Clear the window
    werase(window);

    // Draw a border around the window
    box(window, 0, 0);

    // Display the directory path
    mvwprintw(window, 1, 1, "Directory: %.*s", COLS - 4, directory);

    // Display files in the directory within the visible range
    for (SIZE i = 0; i < files_len; i++) {
    	const char *current_name = files[i];
        // Get the extension-related stuff separated
        const char *extension = strrchr(current_name, '.');

        // Truncate file names that exceed the window width
        [[maybe_unused]]
        SIZE max_display_length = COLS - 6;  // Adjusted to leave space for potential border

        if (i == selected_entry)
            wattron(window, A_REVERSE);

        if (strlen(current_name) > MAX_DISPLAY_LENGTH) {
            if (extension && strlen(extension) <= MAX_DISPLAY_LENGTH - 4) {
                mvwprintw(window, i + 4, 2, "%.*s...%s", MAX_DISPLAY_LENGTH - 7, current_name, extension);
            } else {
                mvwprintw(window, i + 4, 2, "%.*s...", MAX_DISPLAY_LENGTH - 3, current_name);
            }
        } else {
            mvwprintw(window, i + 4, 2, "%s", current_name);

            // Check if it's a directory
            bool is_dir = is_directory(directory, current_name);

            if (is_dir) {
                // Print an indicator for directories
                mvwprintw(window, i + 4, MAX_DISPLAY_LENGTH + 2, "[DIR]");
            } else if (extension) {
                // Print the file extension
                mvwprintw(window, i + 4, MAX_DISPLAY_LENGTH + 2, "%s", extension);
            }
        }

        if (i == selected_entry)
            wattroff(window, A_REVERSE);
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
    cas->cursor = MAX(0, cas->cursor);
    cas->cursor = MIN(cas->cursor, cas->num_files - 1);

    cas->start = MIN(cas->start, cas->cursor);
    cas->start = MAX(cas->start, cas->cursor + 1 - cas->num_lines);
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
    if (strcmp(*current_directory, parent_directory) != 0) {
        free(*current_directory);
        *current_directory = strdup(parent_directory);

        Vector_set_len(files, 0);
        append_files_to_vec(files, *current_directory);
    }
}

void navigate_right(char **current_directory, const char *selected_entry, Vector *files) {
    // Check if the selected entry is a directory
    if (is_directory(*current_directory, selected_entry)) {
        // Change to the selected directory
        char new_path[MAX_PATH_LENGTH];
        // TODO: verify on docs if snprintf always writes the null byte
        snprintf(new_path, sizeof(new_path), "%s/%s", *current_directory, selected_entry);

        free(*current_directory);
        *current_directory = strdup(new_path);

        Vector_set_len(files, 0);
        append_files_to_vec(files, *current_directory);
    }
}



// TODO: make it adapt itself when the screen gets resized
int main() {
    WINDOW *mainwin;
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(1);

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
                    dir_window_cas.cursor    = 0;
                    dir_window_cas.start     = 0;
                    dir_window_cas.num_lines = LINES - 5;
                    dir_window_cas.num_files = Vector_len(files);
                    preview_window_cas.cursor    = 0;
                    preview_window_cas.start     = 0;
                    preview_window_cas.num_lines = LINES - 5;
                    preview_window_cas.num_files = Vector_len(files);
                    break;
                case KEY_RIGHT:
                    // Navigate right (go into the selected directory)
                    navigate_right(
                        &current_directory,
                        files.el[dir_window_cas.cursor],
                        &files
                    );
                    // Reload the file list for the new directory
                    // (similar code as initializing the file list)
                    // ...

                    // FIXME: repeated code
                    dir_window_cas.cursor    = 0;
                    dir_window_cas.start     = 0;
                    dir_window_cas.num_lines = LINES - 5;
                    dir_window_cas.num_files = Vector_len(files);
                    preview_window_cas.cursor    = 0;
                    preview_window_cas.start     = 0;
                    preview_window_cas.num_lines = LINES - 5;
                    preview_window_cas.num_files = Vector_len(files);
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
            (char **)&files.el[dir_window_cas.start],
            // TODO: make sure that its impossible for num_lines to get past
            //       num_files.
            MIN(dir_window_cas.num_lines, dir_window_cas.num_files - dir_window_cas.start),
            dir_window_cas.cursor - dir_window_cas.start
        );

        // Draw the preview window
        draw_preview_window(previewwin, filename, content);

        mvprintw(1, 1, "%d %d %d %d", dir_window_cas.cursor, dir_window_cas.num_files, dir_window_cas.start, dir_window_cas.num_lines);

        // Refresh the main window
        wrefresh(mainwin);
    }


    Vector_bye(&files);
    free(current_directory);

    // Clean up
    endwin();
    return 0;
}
