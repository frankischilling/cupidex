// File: main.c
// -----------------------
// This is the main entry point for the terminal-based file manager (cupidfm).
// It uses ncurses for UI, supports keybindings, and includes directory navigation, 
// file operations, and previews.

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>     // for snprintf
#include <stdlib.h>    // for free, malloc
#include <unistd.h>    // for getenv
#include <ncurses.h>   // for initscr, noecho, cbreak, keypad, curs_set, timeout, endwin, LINES, COLS, getch, timeout, wtimeout, ERR, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_F1, newwin, subwin, box, wrefresh, werase, mvwprintw, wattron, wattroff, A_REVERSE, A_BOLD, getmaxyx, refresh
#include <dirent.h>    // for opendir, readdir, closedir
#include <sys/types.h> // for types like SIZE
#include <sys/stat.h>  // for struct stat
#include <string.h>    // for strlen, strcpy, strdup, strrchr, strtok, strncmp
#include <signal.h>    // for signal, SIGWINCH
#include <stdbool.h>   // for bool, true, false
#include <ctype.h>     // for isspace, toupper
#include <magic.h>     // For libmagic
#include <time.h>      // For strftime, clock_gettime
#include <sys/ioctl.h> // For ioctl
#include <termios.h>   // For resize_term
#include <pthread.h>   // For threading
#include <locale.h>    // For setlocale
#include <errno.h>     // For errno
// Local includes
#include "utils.h"
#include "vector.h"
#include "files.h"
#include "vecstack.h"
#include "main.h"
#include "globals.h"
#include "config.h"
#include "ui.h"

// Global resize flag
volatile sig_atomic_t resized = 0;
volatile sig_atomic_t is_editing = 0;

// Other global windows
WINDOW *mainwin = NULL;
WINDOW *dirwin = NULL;
WINDOW *previewwin = NULL;

VecStack directoryStack;

// Typedefs and Structures
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

// Forward declaration of fix_cursor
void fix_cursor(CursorAndSlice *cas);

// Function Implementations

// Function to draw the directory window
void draw_directory_window(
        WINDOW *window,
        const char *directory,
        Vector *files_vector,
        CursorAndSlice *cas
) {
    int cols;
    int __attribute__((unused)) throwaway;
    getmaxyx(window, throwaway, cols);  // Get window dimensions
    
    // Clear the window and draw border
    werase(window);
    box(window, 0, 0);
    
    // Check if the directory is empty
    if (cas->num_files == 0) {
        mvwprintw(window, 1, 1, "This directory is empty");
        wrefresh(window);
        return;
    }
    
    // Initialize magic for MIME type detection
    magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
    if (magic_cookie == NULL || magic_load(magic_cookie, NULL) != 0) {
        // Fallback to basic directory/file emojis if magic fails
        for (int i = 0; i < cas->num_lines && (cas->start + i) < cas->num_files; i++) {
            FileAttr fa = (FileAttr)files_vector->el[cas->start + i];
            const char *name = FileAttr_get_name(fa);
            const char *emoji = FileAttr_is_dir(fa) ? "📁" : "📄";

            if ((cas->start + i) == cas->cursor) {
                wattron(window, A_REVERSE);
            }

            int name_len = strlen(name);
            int max_name_len = cols - 4; // Adjusted to fit within window width
            if (name_len > max_name_len) {
                mvwprintw(window, i + 1, 1, "%s %.*s...", emoji, max_name_len - 3, name);
            } else {
                mvwprintw(window, i + 1, 1, "%s %s", emoji, name);
            }

            if ((cas->start + i) == cas->cursor) {
                wattroff(window, A_REVERSE);
            }
        }
    } else {
        // Use magic to get proper file type emojis
        for (int i = 0; i < cas->num_lines && (cas->start + i) < cas->num_files; i++) {
            FileAttr fa = (FileAttr)files_vector->el[cas->start + i];
            const char *name = FileAttr_get_name(fa);
            
            // Construct full path for MIME type detection
            char full_path[MAX_PATH_LENGTH];
            path_join(full_path, directory, name);
            
            const char *emoji;
            if (FileAttr_is_dir(fa)) {
                emoji = "📁";
            } else {
                const char *mime_type = magic_file(magic_cookie, full_path);
                emoji = get_file_emoji(mime_type, name);
            }

            if ((cas->start + i) == cas->cursor) {
                wattron(window, A_REVERSE);
            }

            int name_len = strlen(name);
            int max_name_len = cols - 4; // Adjusted to fit within window width
            if (name_len > max_name_len) {
                mvwprintw(window, i + 1, 1, "%s %.*s...", emoji, max_name_len - 3, name);
            } else {
                mvwprintw(window, i + 1, 1, "%s %s", emoji, name);
            }

            if ((cas->start + i) == cas->cursor) {
                wattroff(window, A_REVERSE);
            }
        }
        magic_close(magic_cookie);
    }

    mvwprintw(window, 0, 2, "Directory: %.*s", cols - 13, directory);
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
        wrefresh(window);
        return;
    }
    
    // Display file size with emoji
    char fileSizeStr[20];
    format_file_size(fileSizeStr, file_stat.st_size);
    mvwprintw(window, 2, 2, "📏 File Size: %s", fileSizeStr);

    // Display file permissions with emoji
    char permissions[10];
    snprintf(permissions, sizeof(permissions), "%o", file_stat.st_mode & 0777);
    mvwprintw(window, 3, 2, "🔒 Permissions: %s", permissions);

    // Display last modification time with emoji
    char modTime[50];
    strftime(modTime, sizeof(modTime), "%c", localtime(&file_stat.st_mtime));
    mvwprintw(window, 4, 2, "🕒 Last Modified: %s", modTime);
    
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

        // If the directory is empty, show a message
      
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

                // Replace tabs with spaces
                for (char *p = line; *p; p++) {
                    if (*p == '\t') {
                        *p = ' ';
                    }
                }

                mvwprintw(window, line_num++, 2, "%.*s", max_x - 4, line);
            }

            fclose(file);

            if (line_num < max_y - 1) {
                mvwprintw(window, line_num++, 2, "--------------------------------");
                mvwprintw(window, line_num++, 2, "[End of file]");
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
    // Ensure cursor stays within valid range
    cas->cursor = MIN(cas->cursor, cas->num_files - 1);
    cas->cursor = MAX(0, cas->cursor);

    // Calculate visible window size (subtract 2 for borders)
    int visible_lines = cas->num_lines - 2;

    // Adjust start position to keep cursor visible
    if (cas->cursor < cas->start) {
        cas->start = cas->cursor;
    } else if (cas->cursor >= cas->start + visible_lines) {
        cas->start = cas->cursor - visible_lines + 1;
    }

    // Ensure start position is valid
    cas->start = MIN(cas->start, cas->num_files - visible_lines);
    cas->start = MAX(0, cas->start);
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

    // Ensure windows are created with correct positions
    dirwin = derwin(mainwin, inner_height, dir_win_width - 1, inner_start_y, dir_start_x);
    previewwin = derwin(mainwin, inner_height, preview_win_width, inner_start_y, preview_start_x);

    notifwin = newwin(notif_height, new_cols, new_lines - notif_height, 0);
    box(notifwin, 0, 0);

    // Update cursor and slice state with correct dimensions
    state->dir_window_cas.num_lines = inner_height;
    fix_cursor(&state->dir_window_cas);

    // Draw borders for subwindows
    box(dirwin, 0, 0);
    box(previewwin, 0, 0);

    // Redraw content
    draw_directory_window(
        dirwin,
        state->current_directory,
        &state->files,
        &state->dir_window_cas
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

/** Function to navigate up in the directory window
 *
 * @param cas the cursor and slice state
 * @param files the list of files
 * @param selected_entry the selected entry
 */
void navigate_up(CursorAndSlice *cas, const Vector *files, const char **selected_entry) {
    if (cas->num_files > 0) {
        if (cas->cursor == 0) {
            // Wrap to bottom
            cas->cursor = cas->num_files - 1;
            // Calculate visible window size (subtract 2 for borders)
            int visible_lines = cas->num_lines;
            // Adjust start to show the last page of entries
            cas->start = MAX(0, cas->num_files - visible_lines);
        } else {
            cas->cursor -= 1;
            // Adjust start if cursor would go off screen
            if (cas->cursor < cas->start) {
                cas->start = cas->cursor;
            }
        }
        fix_cursor(cas);
        *selected_entry = FileAttr_get_name(files->el[cas->cursor]);
    }
}

/** Function to navigate down in the directory window
 *
 * @param cas the cursor and slice state
 * @param files the list of files
 * @param selected_entry the selected entry
 */
void navigate_down(CursorAndSlice *cas, const Vector *files, const char **selected_entry) {
    if (cas->num_files > 0) {
        if (cas->cursor >= cas->num_files - 1) {
            // Wrap to top
            cas->cursor = 0;
            cas->start = 0;
        } else {
            cas->cursor += 1;
            // Calculate visible window size (subtract 2 for borders)
            int visible_lines = cas->num_lines;
            
            // Adjust start if cursor would go off screen
            if (cas->cursor >= cas->start + visible_lines) {
                cas->start = cas->cursor - visible_lines + 1;
            }
        }
        fix_cursor(cas);
        *selected_entry = FileAttr_get_name(files->el[cas->cursor]);
    }
}

/** Function to navigate left in the directory window
 *
 * @param current_directory the current directory
 * @param files the list of files
 * @param cas the cursor and slice state
 * @param state the application state
 */
void navigate_left(char **current_directory, Vector *files, CursorAndSlice *dir_window_cas, AppState *state) {
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
    char *popped_dir = VecStack_pop(&directoryStack);
    if (popped_dir) {
        free(popped_dir);
    }

    // Reset cursor and start position
    dir_window_cas->cursor = 0;
    dir_window_cas->start = 0;
    dir_window_cas->num_lines = LINES - 5;
    dir_window_cas->num_files = Vector_len(*files);

    // Set selected_entry to the first file in the parent directory
    if (dir_window_cas->num_files > 0) {
        state->selected_entry = FileAttr_get_name(files->el[0]);
    } else {
        state->selected_entry = "";
    }

    werase(notifwin);
    show_notification(notifwin, "Navigated to parent directory: %s", *current_directory);
    should_clear_notif = false;
    
    wrefresh(notifwin);
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
        show_notification(notifwin, "Selected entry is not a directory");
        should_clear_notif = false;

        wrefresh(notifwin);
        return;
    }

    // Construct the new path carefully
    char new_path[MAX_PATH_LENGTH];
    path_join(new_path, *current_directory, selected_entry);

    // Check if we’re not re-entering the same directory path
    if (strcmp(new_path, *current_directory) == 0) {
        werase(notifwin);
        show_notification(notifwin, "Already in this directory");
        should_clear_notif = false;
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

    // Set selected_entry to the first file in the new directory
    if (dir_window_cas->num_files > 0) {
        state->selected_entry = FileAttr_get_name(files->el[0]);
    } else {
        state->selected_entry = "";
    }

    // If there’s only one entry, automatically select it
    if (dir_window_cas->num_files == 1) {
        state->selected_entry = FileAttr_get_name(files->el[0]);
    }

    werase(notifwin);
    show_notification(notifwin, "Entered directory: %s", state->selected_entry);
    should_clear_notif = false;    
    
    wrefresh(notifwin);
}

/**
 * @brief Handles terminal window resize events.
 *
 * @param sig The signal number.
 */
void handle_winch(int sig) {
    (void)sig;  // Suppress unused parameter warning
    if (!is_editing) {
        resized = 1;
    }
}

/**
 * @brief Main entry point for the application.
 */
int main() {
    // Initialize ncurses
    setlocale(LC_ALL, "");
    
    // Initialize directory stack
    VecStack_init(&directoryStack);

    // Ignore Ctrl+C at the OS level so we can handle it ourselves
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;      // SIG_IGN means "ignore this signal"
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // Set up signal handler for window resize
    sa.sa_handler = handle_winch;
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    sigaction(SIGWINCH, &sa, NULL);

    // Initialize ncurses
    initscr();
    noecho();
    raw();   // or cbreak() if you prefer
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    // Initialize windows and other components
    int notif_height = 1;
    int banner_height = 3;

    // Initialize notif window
    notifwin = newwin(notif_height, COLS, LINES - notif_height, 0);
    werase(notifwin);
    box(notifwin, 0, 0);
    wrefresh(notifwin);

    // Initialize banner window
    bannerwin = newwin(banner_height, COLS, 0, 0);
    // Assuming COLOR_PAIR(1) is defined elsewhere; if not, remove or define it
    // wbkgd(bannerwin, COLOR_PAIR(1)); // Set background color
    box(bannerwin, 0, 0);
    wrefresh(bannerwin);

    // Initialize main window
    mainwin = newwin(LINES - banner_height - notif_height, COLS, banner_height, 0);
    wtimeout(mainwin, 100);

    // Initialize subwindows
    SIZE dir_win_width = MAX(COLS / 2, 20);
    SIZE preview_win_width = MAX(COLS - dir_win_width, 20);

    if (dir_win_width + preview_win_width > COLS) {
        dir_win_width = COLS / 2;
        preview_win_width = COLS - dir_win_width;
    }

    dirwin = subwin(mainwin, LINES - banner_height - notif_height, dir_win_width - 1, banner_height, 0);
    box(dirwin, 0, 0);
    wrefresh(dirwin);

    previewwin = subwin(mainwin, LINES - banner_height - notif_height, preview_win_width, banner_height, dir_win_width);
    box(previewwin, 0, 0);
    wrefresh(previewwin);

    // Initialize keybindings and configs
    KeyBindings kb;
    load_default_keybindings(&kb);

    char config_path[1024];
    const char *home = getenv("HOME");
    if (!home) {
        // Fallback if $HOME is not set
        home = ".";
    }
    snprintf(config_path, sizeof(config_path), "%s/.cupidfmrc", home);

    // Initialize an error buffer to capture error messages
    char error_buffer[ERROR_BUFFER_SIZE] = {0};

    // Load the user configuration, capturing any errors
    int config_errors = load_config_file(&kb, config_path, error_buffer, sizeof(error_buffer));

    if (config_errors == 0) {
        // Configuration loaded successfully
        show_notification(notifwin, "Configuration loaded successfully.");
    } else if (config_errors == 1 && strstr(error_buffer, "Configuration file not found")) {
        // Configuration file not found; create a default config file
        FILE *fp = fopen(config_path, "w");
        if (fp) {
            fprintf(fp, "# CupidFM Configuration File\n");
            fprintf(fp, "# Automatically generated on first run.\n\n");

            // Navigation Keys
            fprintf(fp, "key_up=KEY_UP\n");
            fprintf(fp, "key_down=KEY_DOWN\n");
            fprintf(fp, "key_left=KEY_LEFT\n");
            fprintf(fp, "key_right=KEY_RIGHT\n");
            fprintf(fp, "key_tab=Tab\n");
            fprintf(fp, "key_exit=F1\n");

            // File Management
            fprintf(fp, "key_edit=^E  # Enter edit mode\n");
            fprintf(fp, "key_copy=^C  # Copy selected file\n");
            fprintf(fp, "key_paste=^V  # Paste copied file\n");
            fprintf(fp, "key_cut=^X  # Cut (move) file\n");
            fprintf(fp, "key_delete=^D  # Delete selected file\n");
            fprintf(fp, "key_rename=^R  # Rename file\n");
            fprintf(fp, "key_new=^N  # Create new file\n");
            fprintf(fp, "key_save=^S  # Save changes\n\n");
            fprintf(fp, "key_new_dir=Shift+N  # Create new directory\n");
            // Editing Mode Keys
            fprintf(fp, "edit_up=KEY_UP\n");
            fprintf(fp, "edit_down=KEY_DOWN\n");
            fprintf(fp, "edit_left=KEY_LEFT\n");
            fprintf(fp, "edit_right=KEY_RIGHT\n");
            fprintf(fp, "edit_save=^S # Save in editor\n");
            fprintf(fp, "edit_quit=^Q # Quit editor\n");
            fprintf(fp, "edit_backspace=KEY_BACKSPACE\n");

            fclose(fp);

            // Notify the user about the creation of the default config file
            show_popup("First Run Setup",
                "No config was found.\n"
                "A default config has been created at:\n\n"
                "  %s\n\n"
                "Press any key to continue...",
                config_path);
        } else {
            // Failed to create the config file
            show_notification(notifwin, "Failed to create default configuration file.");
        }
    } else {
        // Configuration file exists but has errors; display the error messages
        show_popup("Configuration Errors",
            "There were issues loading your configuration:\n\n%s\n\n"
            "Press any key to continue with default settings.",
            error_buffer);
        
        // Optionally, you can decide whether to proceed with defaults or halt execution
        // For now, we'll proceed with whatever was loaded and keep defaults for invalid entries
    }

    // Now that keybindings are loaded from config, initialize the banner
    char banner_text_buffer[256];
    snprintf(banner_text_buffer, sizeof(banner_text_buffer), "Welcome to CupidFM - Press %s to exit", keycode_to_string(kb.key_exit));

    // Assign to global BANNER_TEXT
    BANNER_TEXT = banner_text_buffer;

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
    wtimeout(mainwin, INPUT_CHECK_INTERVAL);  // Set shorter timeout for input checking

    // Initialize scrolling variables
    int banner_offset = 0;
    struct timespec last_update_time;
    clock_gettime(CLOCK_MONOTONIC, &last_update_time);

    // Calculate the total scroll length for the banner
    int total_scroll_length = COLS + strlen(BANNER_TEXT) + strlen(BUILD_INFO) + 4;

    int ch;
    while ((ch = getch()) != kb.key_exit) {
        if (resized) {
            resized = 0;
            redraw_all_windows(&state);
            continue;
        }
        // Check if enough time has passed to update the banner
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long time_diff = (current_time.tv_sec - last_update_time.tv_sec) * 1000000 +
                         (current_time.tv_nsec - last_update_time.tv_nsec) / 1000;

        if (time_diff >= BANNER_SCROLL_INTERVAL) {
            // Update banner with current offset
            draw_scrolling_banner(bannerwin, BANNER_TEXT, BUILD_INFO, banner_offset);
            banner_offset = (banner_offset + 1) % total_scroll_length;
            last_update_time = current_time;
        }

        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long notification_diff = (current_time.tv_sec - last_notification_time.tv_sec) * 1000 +
                               (current_time.tv_nsec - last_notification_time.tv_nsec) / 1000000;

        if (!should_clear_notif && notification_diff >= NOTIFICATION_TIMEOUT_MS) {
            werase(notifwin);
            wrefresh(notifwin);
            should_clear_notif = true;
        }

        if (ch != ERR) {

            // 1) UP
            if (ch == kb.key_up) {
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    navigate_up(&state.dir_window_cas, &state.files, &state.selected_entry);
                    state.preview_start_line = 0;
                    werase(notifwin);
                    show_notification(notifwin, "Moved up");
                    wrefresh(notifwin);
                    should_clear_notif = false;
                } else if (active_window == PREVIEW_WIN_ACTIVE) {
                    if (state.preview_start_line > 0) {
                        state.preview_start_line--;
                        werase(notifwin);
                        show_notification(notifwin, "Scrolled up");
                        wrefresh(notifwin);
                        should_clear_notif = false;
                    }
                }
            }

            // 2) DOWN
            else if (ch == kb.key_down) {
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    navigate_down(&state.dir_window_cas, &state.files, &state.selected_entry);
                    state.preview_start_line = 0;
                    werase(notifwin);
                    show_notification(notifwin, "Moved down");
                    wrefresh(notifwin);
                    should_clear_notif = false;
                } else if (active_window == PREVIEW_WIN_ACTIVE) {
                    // Determine total lines for scrolling in the preview
                    char file_path[MAX_PATH_LENGTH];
                    path_join(file_path, state.current_directory, state.selected_entry);
                    int total_lines = get_total_lines(file_path);

                    int max_x, max_y;
                    getmaxyx(previewwin, max_y, max_x);
                    (void) max_x;
                    int content_height = max_y - 7;
                    int max_start_line = total_lines - content_height;
                    if (max_start_line < 0) max_start_line = 0;

                    if (state.preview_start_line < max_start_line) {
                        state.preview_start_line++;
                        werase(notifwin);
                        show_notification(notifwin, "Scrolled down");
                        wrefresh(notifwin);
                        should_clear_notif = false;
                    }
                }
            }

            // 3) LEFT
            else if (ch == kb.key_left) {
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    navigate_left(&state.current_directory,
                                &state.files,
                                &state.dir_window_cas,
                                &state);
                    state.preview_start_line = 0;
                    werase(notifwin);
                    show_notification(notifwin, "Navigated to parent directory");
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
            }

            // 4) RIGHT
            else if (ch == kb.key_right) {
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    if (FileAttr_is_dir(state.files.el[state.dir_window_cas.cursor])) {
                        navigate_right(&state,
                                    &state.current_directory,
                                    state.selected_entry,
                                    &state.files,
                                    &state.dir_window_cas);
                        state.preview_start_line = 0;

                        // If there's only one file in the directory, auto-select it
                        if (state.dir_window_cas.num_files == 1) {
                            state.selected_entry = FileAttr_get_name(state.files.el[0]);
                        }
                        werase(notifwin);
                        show_notification(notifwin, "Entered directory: %s", state.selected_entry);
                        wrefresh(notifwin);
                        should_clear_notif = false;
                    } else {
                        werase(notifwin);
                        show_notification(notifwin, "Selected entry is not a directory");
                        wrefresh(notifwin);
                        should_clear_notif = false;
                    }
                }
            }

            // 5) TAB (switch active window)
            else if (ch == kb.key_tab) {
                active_window = (active_window == DIRECTORY_WIN_ACTIVE)
                                ? PREVIEW_WIN_ACTIVE
                                : DIRECTORY_WIN_ACTIVE;
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    state.preview_start_line = 0;
                }
                werase(notifwin);
                show_notification(
                    notifwin,
                    "Switched to %s window",
                    (active_window == DIRECTORY_WIN_ACTIVE) ? "Directory" : "Preview"
                );
                wrefresh(notifwin);
                should_clear_notif = false;
            }

            // 6) EDIT 
            else if (ch == kb.key_edit) {
                if (active_window == PREVIEW_WIN_ACTIVE) {
                    char file_path[MAX_PATH_LENGTH];
                    path_join(file_path, state.current_directory, state.selected_entry);
                    edit_file_in_terminal(previewwin, file_path, notifwin, &kb);
                    state.preview_start_line = 0;
                    werase(notifwin);
                    show_notification(notifwin, "Editing file: %s", state.selected_entry);
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
            }

            // 7) COPY
            else if (ch == kb.key_copy) {
                if (active_window == DIRECTORY_WIN_ACTIVE && state.selected_entry) {
                    char full_path[MAX_PATH_LENGTH];
                    path_join(full_path, state.current_directory, state.selected_entry);
                    copy_to_clipboard(full_path);
                    strncpy(copied_filename, state.selected_entry, MAX_PATH_LENGTH);
                    werase(notifwin);
                    show_notification(notifwin, "Copied to clipboard: %s", state.selected_entry);
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
            }

            // 8) PASTE
            else if (ch == kb.key_paste) {
                if (active_window == DIRECTORY_WIN_ACTIVE && copied_filename[0] != '\0') {
                    paste_from_clipboard(state.current_directory, copied_filename);
                    reload_directory(&state.files, state.current_directory);
                    state.dir_window_cas.num_files = Vector_len(state.files);
                    werase(notifwin);
                    show_notification(notifwin, "Pasted file: %s", copied_filename);
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
            }

            // 9) CUT 
            else if (ch == kb.key_cut) {
                if (active_window == DIRECTORY_WIN_ACTIVE && state.selected_entry) {
                    char full_path[MAX_PATH_LENGTH];
                    path_join(full_path, state.current_directory, state.selected_entry);
                    cut_and_paste(full_path);
                    strncpy(copied_filename, state.selected_entry, MAX_PATH_LENGTH);

                    // Reload directory to reflect the cut file
                    reload_directory(&state.files, state.current_directory);
                    state.dir_window_cas.num_files = Vector_len(state.files);

                    werase(notifwin);
                    show_notification(notifwin, "Cut to clipboard: %s", state.selected_entry);
                    wrefresh(notifwin);
                    should_clear_notif = false;
                }
            }

            // 10) DELETE
            else if (ch == kb.key_delete) {
                if (active_window == DIRECTORY_WIN_ACTIVE && state.selected_entry) {
                    char full_path[MAX_PATH_LENGTH];
                    path_join(full_path, state.current_directory, state.selected_entry);

                    bool should_delete = false;
                    bool delete_result = confirm_delete(state.selected_entry, &should_delete);

                    if (delete_result && should_delete) {
                        delete_item(full_path);
                        reload_directory(&state.files, state.current_directory);
                        state.dir_window_cas.num_files = Vector_len(state.files);

                        show_notification(notifwin, "Deleted: %s", state.selected_entry);
                        should_clear_notif = false;
                    } else {
                        show_notification(notifwin, "Delete cancelled");
                        should_clear_notif = false;
                    }
                }
            }


            // 11) RENAME
            else if (ch == kb.key_rename) {
                if (active_window == DIRECTORY_WIN_ACTIVE && state.selected_entry) {
                    char full_path[MAX_PATH_LENGTH];
                    path_join(full_path, state.current_directory, state.selected_entry);

                    rename_item(notifwin, full_path);

                    // Reload to show changes
                    reload_directory(&state.files, state.current_directory);
                    state.dir_window_cas.num_files = Vector_len(state.files);

                    if (state.dir_window_cas.num_files > 0) {
                        state.dir_window_cas.cursor = 0;
                        state.dir_window_cas.start = 0;
                        state.selected_entry = FileAttr_get_name(state.files.el[0]);
                    } else {
                        state.selected_entry = "";
                    }
                }
            }

            // 12) CREATE NEW 
            else if (ch == kb.key_new) {
                if (active_window == DIRECTORY_WIN_ACTIVE) {
                    create_new_file(notifwin, state.current_directory);
                    reload_directory(&state.files, state.current_directory);
                    state.dir_window_cas.num_files = Vector_len(state.files);

                    if (state.dir_window_cas.num_files > 0) {
                        state.dir_window_cas.cursor = 0;
                        state.dir_window_cas.start = 0;
                        state.selected_entry = FileAttr_get_name(state.files.el[0]);
                    } else {
                        state.selected_entry = "";
                    }
                }
            }

            // 13 CREATE NEW DIR
            else if (ch == kb.key_new_dir) {
                // implement
                create_new_directory(notifwin, state.current_directory);
                reload_directory(&state.files, state.current_directory);
                state.dir_window_cas.num_files = Vector_len(state.files);

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
                &state.files,
                &state.dir_window_cas
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
    delwin(bannerwin);
    endwin();
    cleanup_temp_files();

    // Destroy directory stack
    VecStack_bye(&directoryStack);

    return 0;
}