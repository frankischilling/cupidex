// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>    // For boolean type support
#include <stddef.h>     // For size_t type

/**
 * @struct KeyBindings
 * @brief Represents the key bindings configuration for the application.
 *
 * This structure holds the key codes for various actions within the application,
 * allowing for customization of controls. It includes bindings for navigation,
 * editing operations, and special functions.
 */
typedef struct {
    // Navigation keys using ncurses key codes
    int key_up;       /**< Key code for navigating up */
    int key_down;     /**< Key code for navigating down */
    int key_left;     /**< Key code for navigating left */
    int key_right;    /**< Key code for navigating right */
    int key_tab;      /**< Key code for the Tab key */
    int key_exit;     /**< Key code for exiting the application */

    // Editing operation keys using ASCII control codes or ncurses key codes
    int key_edit;     /**< Key code for entering edit mode (e.g., Ctrl+E) */
    int key_copy;     /**< Key code for copying content (e.g., Ctrl+C) */
    int key_paste;    /**< Key code for pasting content (e.g., Ctrl+V) */
    int key_cut;      /**< Key code for cutting content (e.g., Ctrl+X) */
    int key_delete;   /**< Key code for deleting content (e.g., Ctrl+D) */
    int key_rename;   /**< Key code for renaming items (e.g., Ctrl+R) */
    int key_new;      /**< Key code for creating new items (e.g., Ctrl+N) */
    int key_save;     /**< Key code for saving changes (e.g., Ctrl+S) */
    int key_new_dir;  /**< Key code for creating new directories (e.g., Shift+N) */

    // Dedicated editing mode keys using ncurses key codes or ASCII control codes
    int edit_up;        /**< Key code for moving up in edit mode */
    int edit_down;      /**< Key code for moving down in edit mode */
    int edit_left;      /**< Key code for moving left in edit mode */
    int edit_right;     /**< Key code for moving right in edit mode */
    int edit_save;      /**< Key code for saving in edit mode (e.g., Ctrl+S) */
    int edit_quit;      /**< Key code for quitting edit mode (e.g., Ctrl+Q) */
    int edit_backspace; /**< Key code for backspacing in edit mode (e.g., Backspace key) */
} KeyBindings;

/**
 * @brief Initializes the KeyBindings structure with default keybindings.
 *
 * This function sets all fields of the provided KeyBindings structure to predefined
 * default values. These defaults are typically standard navigation and editing keys.
 *
 * @param kb Pointer to the KeyBindings structure to initialize.
 */
void load_default_keybindings(KeyBindings *kb);

/**
 * @brief Loads user-defined keybindings from a configuration file, overriding defaults.
 * 
 * This function reads a configuration file specified by `filepath`, parses key-value
 * pairs representing keybindings, and updates the provided KeyBindings structure accordingly.
 * It handles comments, trims whitespace, and reports any errors encountered during parsing.
 *
 * @param kb           Pointer to the KeyBindings structure to populate or update.
 * @param filepath     Path to the configuration file containing user-defined keybindings.
 * @param error_buffer Buffer to store error messages generated during loading.
 * @param buffer_size  Size of the `error_buffer` to prevent overflow.
 * @return             The number of errors encountered during the loading process.
 *                     A return value of 0 indicates successful loading without errors.
 */
int load_config_file(KeyBindings *kb, const char *filepath, char *error_buffer, size_t buffer_size);

#endif // CONFIG_H
