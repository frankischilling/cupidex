// config.h
#ifndef CONFIG_H
#define CONFIG_H

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

    // Dedicated editing keys
    int edit_up;
    int edit_down;
    int edit_left;
    int edit_right;
    int edit_save;
    int edit_quit;
    int edit_backspace;
} KeyBindings;

/**
 * Loads defaults if config file does not exist or if particular
 * bindings are not overridden by user. 
 */
void load_default_keybindings(KeyBindings *kb);

/**
 * Attempt to parse a config file (e.g., "~/.cupidfmrc") and override
 * the defaults. If the file does not exist or is invalid, do nothing.
 *
 * Return `true` on success, `false` if file is not found or invalid.
 */
bool load_config_file(KeyBindings *kb, const char *filepath);

#endif // CONFIG_H
