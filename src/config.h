// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int key_up;
    int key_down;
    int key_left;
    int key_right;
    int key_tab;
    int key_exit;

    int key_edit;    // e.g., Ctrl+E
    int key_copy;    // e.g., Ctrl+C
    int key_paste;   // e.g., Ctrl+V
    int key_cut;     // e.g., Ctrl+X
    int key_delete;  // e.g., Ctrl+D
    int key_rename;  // e.g., Ctrl+R
    int key_new;     // e.g., Ctrl+N
    int key_save;    // e.g., Ctrl+S
    int key_new_dir; // e.g., Shift+N

    // Dedicated editing keys
    int edit_up;
    int edit_down;
    int edit_left;
    int edit_right;
    int edit_save;
    int edit_quit;
    int edit_backspace;

    // file 
    int info_label_width;
} KeyBindings;

/**
 * Loads default keybindings into the provided KeyBindings structure.
 */
void load_default_keybindings(KeyBindings *kb);

/**
 * Loads user configuration from a file, overriding defaults.
 * 
 * @param kb           Pointer to KeyBindings structure to populate.
 * @param filepath     Path to the configuration file.
 * @param error_buffer Buffer to store error messages.
 * @param buffer_size  Size of the error_buffer.
 * @return             Number of errors encountered. 0 indicates success.
 */
int load_config_file(KeyBindings *kb, const char *filepath, char *error_buffer, size_t buffer_size);

#endif // CONFIG_H
