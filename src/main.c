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
#include <string.h>    // For memset
#include <magic.h>     // For libmagic
#include <time.h>      // For strftime
#include <sys/ioctl.h> // For ioctl
#include <termios.h>   // For resize_term
// Local includes
#include "utils.h"
#include "vector.h"
#include "files.h"
#include "vecstack.h"

#define MAX_PATH_LENGTH 256 // 256
#define TAB 9
#define CTRL_E 5

#define BANNER_TEXT "  CupidFM - Terminal File Manager  "
#define BUILD_INFO "  Build: 2024-11-08  "

// Global resize flag
volatile sig_atomic_t resized = 0;

// Global variables
WINDOW *notifwin;
WINDOW *mainwin;
WINDOW *dirwin;
WINDOW *previewwin;
WINDOW *bannerwin;

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

/** Function to show directory tree recursively
 *
 * @param window the window to display the directory tree
 * @param dir_path the path of the directory to display
 * @param level the current level of the directory tree
 * @param line_num the current line number in the window
 * @param max_y the maximum number of lines in the window
 * @param max_x the maximum number of columns in the window
 */
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
/** Function to update the directory stack with the new directory
 *
 * @param newDirectory the new directory to push onto the stack
 */
void updateDirectoryStack(const char *newDirectory) {
    char *token;
    char *copy = strdup(newDirectory);

    // Push each directory onto the stack
    for (token = strtok(copy, "/"); token; token = strtok(NULL, "/")) {
        VecStack_push(&directoryStack, strdup(token));
    }

    free(copy);
}
/** Function to draw the directory window
 *
 * @param window the window to draw the directory in
 * @param directory the current directory
 * @param files the list of files in the directory
 * @param files_len the number of files in the directory
 * @param selected_entry the index of the selected entry
 */
bool is_hidden(const char *filename) {
    return filename[0] == '.' && (strlen(filename) == 1 || (filename[1] != '.' && filename[1] != '\0'));
}
/** Function to get the total number of lines in a file
 *
 * @param file_path the path to the file
 * @return the total number of lines in the file
 */

/** Function to get the total number of lines in a file
 *
 * @param file_path
 * @return total_lines number of lines in the file
 */
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
/** Function to draw the directory window
 *
 * @param window the window to draw the directory in
 * @param directory the current directory
 * @param files the list of files in the directory
 * @param files_len the number of files in the directory
 * @param selected_entry the index of the selected entry
 */
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
/** Function to draw the preview window
 *
 * @param window the window to draw the preview in
 * @param current_directory the current directory
 * @param selected_entry the selected entry
 * @param start_line the starting line of the preview
 */
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
/** Function to handle cursor movement in the directory window
 * @param cas the cursor and slice state
 */
void fix_cursor(CursorAndSlice *cas) {
    cas->cursor = MIN(cas->cursor, cas->num_files - 1);
    cas->cursor = MAX(0, cas->cursor);

    cas->start = MIN(cas->start, cas->cursor);
    cas->start = MAX(cas->start, cas->cursor + 1 - cas->num_lines);
}
/** Function to redraw all windows
 *
 * @param state the application state
 */
void redraw_all_windows(AppState *state) {
    // Get new terminal dimensions
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    resize_term(w.ws_row, w.ws_col);

    // Update ncurses internal structures
    endwin();
    refresh();
    clear();

    // Recalculate window dimensions with minimum sizes
    int new_cols = MAX(COLS, 40);  // Minimum width of 40 columns
    int new_lines = MAX(LINES, 10); // Minimum height of 10 lines
    int banner_height = 3;
    int notif_height = 1;
    int main_height = new_lines - banner_height - notif_height;

    // Calculate subwindow dimensions with minimum sizes
    SIZE dir_win_width = MAX(new_cols / 3, 20);  // Minimum directory window width
    SIZE preview_win_width = new_cols - dir_win_width - 2; // Account for borders

    // Delete all windows first
    if (dirwin) delwin(dirwin);
    if (previewwin) delwin(previewwin);
    if (mainwin) delwin(mainwin);
    if (bannerwin) delwin(bannerwin);
    if (notifwin) delwin(notifwin);

    // Recreate all windows in order
    bannerwin = newwin(banner_height, new_cols, 0, 0);
    box(bannerwin, 0, 0);

    mainwin = newwin(main_height, new_cols, banner_height, 0);
    box(mainwin, 0, 0);

    // Create subwindows with proper border accounting
    int inner_height = main_height - 2;  // Account for main window borders
    int inner_start_y = 1;               // Start after main window's top border
    int dir_start_x = 1;                 // Start after main window's left border
    int preview_start_x = dir_win_width + 1; // Start after directory window

    dirwin = derwin(mainwin, inner_height, dir_win_width - 1, inner_start_y, dir_start_x);
    previewwin = derwin(mainwin, inner_height, preview_win_width, inner_start_y, preview_start_x);

    notifwin = newwin(notif_height, new_cols, new_lines - notif_height, 0);
    box(notifwin, 0, 0);

    // Update cursor and slice state
    state->dir_window_cas.num_lines = inner_height;
    fix_cursor(&state->dir_window_cas);

    // Draw borders for subwindows
    box(dirwin, 0, 0);
    box(previewwin, 0, 0);

    // Redraw content
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

    // Refresh all windows in correct order
    refresh();
    wrefresh(bannerwin);
    wrefresh(mainwin);
    wrefresh(dirwin);
    wrefresh(previewwin);
    wrefresh(notifwin);
}

/** Function to reload the directory contents
 *
 * @param files the list of files
 * @param current_directory the current directory
 */
void reload_directory(Vector *files, const char *current_directory) {
    // Empties the vector
    Vector_set_len(files, 0);
    // Reads the filenames
    append_files_to_vec(files, current_directory);
    // Makes the vector shorter
    Vector_sane_cap(files);
}
/** Function to navigate up in the directory window
 *
 * @param cas the cursor and slice state
 * @param files the list of files
 * @param selected_entry the selected entry
 */
void navigate_up(CursorAndSlice *cas, const Vector *files, const char **selected_entry) {
    if (cas->num_files > 1) {
        cas->cursor -= 1;
        fix_cursor(cas);
    }
    *selected_entry = FileAttr_get_name(files->el[cas->cursor]);
}
/** Function to navigate down in the directory window
 *
 * @param cas the cursor and slice state
 * @param files the list of files
 * @param selected_entry the selected entry
 */
void navigate_down(CursorAndSlice *cas, const Vector *files, const char **selected_entry) {
    if (cas->num_files > 1) {
        cas->cursor += 1;
        fix_cursor(cas);
    }
    *selected_entry = FileAttr_get_name(files->el[cas->cursor]);
}
/** Function to navigate left in the directory window
 *
 * @param current_directory the current directory
 * @param files the list of files
 * @param cas the cursor and slice state
 */
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
/** Function to navigate right in the directory window
 *
 * @param state the application state
 * @param current_directory the current directory
 * @param selected_entry the selected entry
 * @param files the list of files
 * @param dir_window_cas the cursor and slice state
 */
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
/** Function to handle terminal window resize
 *
 * @param sig the signal number
 */
void handle_winch(int sig) {
    (void)sig;  // Suppress unused parameter warning
    resized = 1;
}
/**
 * Function to draw and scroll the banner text
 *
 * @param window the banner window
 * @param text the text to scroll
 * @param build_info the build information to display
 * @param offset the current offset for scrolling
 */
void draw_scrolling_banner(WINDOW *window, const char *text, const char *build_info, int offset) {
    int width = COLS - 2; // Adjust for border
    int text_len = strlen(text);
    int build_len = strlen(build_info);

    // Calculate total length including padding
    int total_len = width + text_len + build_len + 4; // +4 for spacing between text and build_info

    // Create the scroll text buffer with double the content for smooth wrapping
    char *scroll_text = malloc(2 * total_len + 1);
    if (!scroll_text) return;

    // Fill the buffer with spaces
    memset(scroll_text, ' ', 2 * total_len);
    scroll_text[2 * total_len] = '\0';

    // Copy the text pattern twice for smooth wrapping
    for (int i = 0; i < 2; i++) {
        int pos = i * total_len;
        memcpy(scroll_text + pos, text, text_len);
        memcpy(scroll_text + pos + text_len + 2, build_info, build_len);
    }

    // Draw the banner
    werase(window);
    box(window, 0, 0);
    
    // Display the scrolling text
    offset = offset % total_len;
    mvwprintw(window, 1, 1, "%.*s", width, scroll_text + offset);
    
    wrefresh(window);
    free(scroll_text);
}

/** Function to handle cleanup and exit
 *
 * @param r the exit code
 * @param format the error message format
 */
int main() {
    // Initialize ncurses
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);
    int notif_height = 1;
    int banner_height = 3; 

    // Initialize  notif windows
    notifwin = newwin(1, COLS, LINES - 1, 0);
    werase(notifwin);
    box(notifwin, 0, 0);
    wrefresh(notifwin);

	// init main window
    mainwin = newwin(LINES - 1, COLS, 0, 0);
    wtimeout(mainwin, 100);

 	// Initialize banner window
    bannerwin = newwin(banner_height, COLS, 0, 0);
    wbkgd(bannerwin, COLOR_PAIR(1)); // Set background color
    box(bannerwin, 0, 0);
    wrefresh(bannerwin);

	// Initialize notifwin below the banner
	notifwin = newwin(notif_height, COLS, banner_height + LINES - 1 - notif_height, 0);
	werase(notifwin);
	box(notifwin, 0, 0);
	wrefresh(notifwin);

	// Initialize main window below the banner and above notifwin
	mainwin = newwin(LINES - banner_height - notif_height, COLS, banner_height, 0);
	wtimeout(mainwin, 100);

	// Initialize subwindows
	SIZE dir_win_width = MAX(COLS / 2, 20);
	SIZE preview_win_width = MAX(COLS - dir_win_width, 20);

	if (dir_win_width + preview_win_width > COLS) {
	    dir_win_width = COLS / 2;
	    preview_win_width = COLS - dir_win_width;
	}

	dirwin = subwin(mainwin, LINES - banner_height - notif_height, dir_win_width, banner_height, 0);
	box(dirwin, 0, 0);
	wrefresh(dirwin);

	previewwin = subwin(mainwin, LINES - banner_height - notif_height, preview_win_width, banner_height, dir_win_width);
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

	// Set a separate timeout for mainwin to handle scrolling
	wtimeout(mainwin, 50); // Adjust as needed

    // Initialize scrolling variables
    int banner_offset = 0;
    int banner_speed = 100; // in milliseconds

	// Calculate the total scroll length for the banner
	int total_scroll_length = COLS + strlen(BANNER_TEXT) + strlen(BUILD_INFO) + 4;

	int ch;
    while ((ch = getch()) != KEY_F(1)) {
        if (resized) {
            redraw_all_windows(&state);
            resized = 0;
        }

 		// Update banner offset and handle scrolling
    	banner_offset = (banner_offset + 1) % total_scroll_length;
    	draw_scrolling_banner(bannerwin, BANNER_TEXT, BUILD_INFO, banner_offset);

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

		// Sleep or delay to control scrolling speed
    	usleep(banner_speed * 1000);

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
