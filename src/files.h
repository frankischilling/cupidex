// files.h
#ifndef FILES_H
#define FILES_H

#include <curses.h>     // For ncurses window handling and key definitions
#include "config.h"     // For KeyBindings structure and related functions
#include "vector.h"     // For dynamic array (Vector) operations

// Define the maximum length for filenames. Although many systems support up to 256 characters,
// this value is set to 512 to accommodate longer filenames if necessary.
#define MAX_FILENAME_LEN 512

/**
 * @struct FileAttributes
 * @brief Represents the attributes of a file.
 *
 * This structure holds metadata about a file, such as its name and type (directory or not).
 * It is defined elsewhere and is used via a pointer in this header to encapsulate file information.
 */
typedef struct FileAttributes* FileAttr;

/**
 * @brief Retrieves the name of the file associated with the given FileAttr.
 *
 * This function returns a constant string representing the filename stored within the
 * provided FileAttributes structure.
 *
 * @param fa Pointer to the FileAttributes structure.
 * @return A constant character pointer to the filename.
 */
const char *FileAttr_get_name(FileAttr fa);

/**
 * @brief Determines if the file is a directory.
 *
 * This function checks the FileAttributes structure to ascertain whether the associated
 * file is a directory.
 *
 * @param fa Pointer to the FileAttributes structure.
 * @return `true` if the file is a directory, `false` otherwise.
 */
bool FileAttr_is_dir(FileAttr fa);

/**
 * @brief Appends a file to the provided Vector.
 *
 * This function adds a filename to a dynamic array (Vector), facilitating the storage
 * and management of multiple filenames.
 *
 * @param v    Pointer to the Vector where the filename will be appended.
 * @param name The name of the file to append.
 */
void append_files_to_vec(Vector *v, const char *name);

/**
 * @brief Displays information about a file in the specified window.
 *
 * This function outputs file-related information (such as name, size, type) to a given
 * ncurses WINDOW. The `max_x` parameter can be used to format the output within the window's width.
 *
 * @param window    Pointer to the ncurses WINDOW where the information will be displayed.
 * @param file_path The path to the file whose information is to be displayed.
 * @param max_x     The maximum horizontal position (width) for formatting the display.
 */
void display_file_info(WINDOW *window, const char *file_path, int max_x);

/**
 * @brief Checks if a file has a supported file type based on its extension.
 *
 * This function examines the filename's extension to determine if it matches any of the
 * application's supported file types. This is useful for filtering files during operations
 * like opening or editing.
 *
 * @param filename The name of the file to check.
 * @return `true` if the file type is supported, `false` otherwise.
 */
bool is_supported_file_type(const char *filename);

/**
 * @brief Opens and edits a file within the terminal using ncurses.
 *
 * This function provides a terminal-based editor for the specified file. It utilizes
 * ncurses windows for the main editing interface and notifications. The keybindings
 * used during editing are provided via the KeyBindings structure, allowing for customized controls.
 *
 * @param window    Pointer to the main ncurses WINDOW where the file will be edited.
 * @param file_path The path to the file to be edited.
 * @param notifwin  Pointer to an ncurses WINDOW used for displaying notifications or messages.
 * @param kb        Pointer to the KeyBindings structure containing key configurations.
 */
void edit_file_in_terminal(WINDOW *window, const char *file_path, WINDOW *notifwin, KeyBindings *kb);

/**
 * @brief Formats the size of a file into a human-readable string.
 *
 * This utility function converts a file size (in bytes) into a formatted string with
 * appropriate units (e.g., KB, MB, GB). The formatted string is stored in the provided buffer.
 *
 * @param buffer The buffer where the formatted file size string will be stored.
 * @param size   The size of the buffer to prevent overflow.
 * @return A pointer to the buffer containing the formatted file size string.
 */
char* format_file_size(char *buffer, size_t size);

#endif // FILES_H
