#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <curses.h>
#include <ncurses.h>

#include "vector.h"
#include "vecstack.h"

// ─────────────────────────────────────────────────────────────
// Type Definitions
// ─────────────────────────────────────────────────────────────

#define SIZE int  // Defines SIZE as an alias for int

// ─────────────────────────────────────────────────────────────
// Utility Macros
// ─────────────────────────────────────────────────────────────

#define MAX(x, y) (((x) > (y)) ? (x) : (y))  // Returns the maximum of two values
#define MIN(a, b) (((a) < (b)) ? (a) : (b))  // Returns the minimum of two values

// ─────────────────────────────────────────────────────────────
// Error Handling
// ─────────────────────────────────────────────────────────────

/**
 * Terminates the program with an error message.
 * @param r Exit code.
 * @param format Formatted error message.
 */
__attribute__((noreturn))
void die(int r, const char *format, ...);

// ─────────────────────────────────────────────────────────────
// File and Directory Operations
// ─────────────────────────────────────────────────────────────

/**
 * Checks if the given filename in the specified path is a directory.
 * @param path The directory path.
 * @param filename The name of the file to check.
 * @return true if it's a directory, false otherwise.
 */
bool is_directory(const char *path, const char *filename);

/**
 * Creates a new file with the given filename.
 * @param filename The name of the file to create.
 */
void create_file(const char *filename);

/**
 * Displays a list of files in the specified directory.
 * @param directory The directory to list files from.
 */
void display_files(const char *directory);

/**
 * Displays a preview of the given file.
 * @param filename The file to preview.
 */
void preview_file(const char *filename);

/**
 * Reloads the directory contents into the given vector.
 * @param files A pointer to a vector that will store the directory contents.
 * @param current_directory The directory to load files from.
 */
void reload_directory(Vector *files, const char *current_directory);

// ─────────────────────────────────────────────────────────────
// Path and String Utilities
// ─────────────────────────────────────────────────────────────

/**
 * Joins two path components into a single path.
 * @param result The resulting path.
 * @param base The base path.
 * @param extra The additional path component.
 */
void path_join(char *result, const char *base, const char *extra);

/**
 * Determines an appropriate emoji for a file based on its MIME type.
 * @param mime_type The MIME type of the file.
 * @param filename The file name.
 * @return A string containing an emoji representation.
 */
const char* get_file_emoji(const char *mime_type, const char *filename);

/**
 * Checks if the given filename is hidden.
 * @param filename The filename to check.
 * @return true if the file is hidden, false otherwise.
 */
bool is_hidden(const char *filename);

/**
 * Converts a keycode into a readable string representation.
 * @param keycode The keycode to convert.
 * @return A string representation of the keycode.
 */
const char* keycode_to_string(int keycode);

/**
 * Gets the total number of lines in a file.
 * @param file_path The path to the file.
 * @return The total number of lines in the file.
 */
int get_total_lines(const char *file_path);

/**
 * Formats a file size into a human-readable string.
 * @param buffer The buffer to store the formatted string.
 * @param size The file size in bytes.
 * @return The formatted file size string.
 */
char* format_file_size(char *buffer, size_t size);

// ─────────────────────────────────────────────────────────────
// Clipboard and File Operations
// ─────────────────────────────────────────────────────────────

/**
 * Copies a file to the clipboard.
 * @param path The path to the file to copy.
 */
void copy_to_clipboard(const char *path);

/**
 * Pastes a file from the clipboard into a target directory.
 * @param target_directory The directory where the file should be pasted.
 * @param filename The filename to use for the pasted file.
 */
void paste_from_clipboard(const char *target_directory, const char *filename);

/**
 * Moves a file by cutting it and pasting it elsewhere.
 * @param path The path of the file to cut.
 */
void cut_and_paste(const char *path);

/**
 * Deletes a file or directory.
 * @param path The path to the file or directory to delete.
 */
void delete_item(const char *path);

/**
 * Prompts the user for confirmation before deleting a file.
 * @param path The file path to confirm deletion for.
 * @param should_delete A pointer to a boolean indicating the user's choice.
 * @return true if the file should be deleted, false otherwise.
 */
bool confirm_delete(const char *path, bool *should_delete);

/**
 * Renames a file or directory.
 * @param notifwin The notification window to display messages in.
 * @param old_path The original file path.
 * @return true if the rename was successful, false otherwise.
 */
bool rename_item(WINDOW *notifwin, const char *old_path);

/**
 * Creates a new file in the specified directory.
 * @param win The window to display messages in.
 * @param dir_path The directory where the new file should be created.
 * @return true if the file was successfully created, false otherwise.
 */
bool create_new_file(WINDOW *win, const char *dir_path);

/**
 * Creates a new directory in the specified path.
 * @param win The window to display messages in.
 * @param dir_path The directory where the new directory should be created.
 * @return true if the directory was successfully created, false otherwise.
 */
bool create_new_directory(WINDOW *win, const char *dir_path);

// ─────────────────────────────────────────────────────────────
// Directory Tree and UI Utilities
// ─────────────────────────────────────────────────────────────

/**
 * Displays a directory tree structure in a given window.
 * @param window The window to display the directory tree in.
 * @param dir_path The root directory to start from.
 * @param level The current depth level.
 * @param line_num The line number to start drawing at.
 * @param max_y The maximum height of the window.
 * @param max_x The maximum width of the window.
 */
void show_directory_tree(WINDOW *window, const char *dir_path, int level, int *line_num, int max_y, int max_x);

// ─────────────────────────────────────────────────────────────
// Banner and Cleanup Utilities
// ─────────────────────────────────────────────────────────────

/**
 * Starts a separate thread to handle the scrolling banner.
 * @param arg The argument to pass to the thread.
 * @return A pointer to the thread's return value.
 */
void *banner_scrolling_thread(void *arg);

/**
 * Draws a scrolling banner in the specified window.
 * @param window The window to display the banner in.
 * @param text The main text of the banner.
 * @param build_info Additional build information to display.
 * @param offset The current offset for scrolling.
 */
void draw_scrolling_banner(WINDOW *window, const char *text, const char *build_info, int offset);

/**
 * Cleans up temporary files created during runtime.
 */
void cleanup_temp_files();

#endif // UTILS_H
