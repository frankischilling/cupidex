// config.c
#include "config.h"
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <ncurses.h>

static int parse_key(const char *val);

/**
 * If you do not find a user config, or an entry is missing from the config,
 * fallback to these defaults.
 */
void load_default_keybindings(KeyBindings *kb) {
    kb->key_up      = KEY_UP;
    kb->key_down    = KEY_DOWN;
    kb->key_left    = KEY_LEFT;
    kb->key_right   = KEY_RIGHT;
    kb->key_tab     = '\t';       // Tab
    kb->key_exit    = KEY_F(1);   // F1

    kb->key_edit    = 5;    // Ctrl+E (ASCII 5)
    kb->key_copy    = 3;    // Ctrl+C (ASCII 3)
    kb->key_paste   = 22;   // Ctrl+V (ASCII 22)
    kb->key_cut     = 24;   // Ctrl+X (ASCII 24)
    kb->key_delete  = 4;    // Ctrl+D (ASCII 4)
    kb->key_rename  = 18;   // Ctrl+R (ASCII 18)
    kb->key_new     = 14;   // Ctrl+N (ASCII 14)
    kb->key_save    = 19;   // Ctrl+S (ASCII 19)

    // Editing mode keys
    kb->edit_up        = KEY_UP;
    kb->edit_down      = KEY_DOWN;
    kb->edit_left      = KEY_LEFT;
    kb->edit_right     = KEY_RIGHT;
    kb->edit_save      = 19;  // Ctrl+S
    kb->edit_quit      = 17;  // Ctrl+Q
    kb->edit_backspace = KEY_BACKSPACE; // ASCII Backspace
}

/**
 * Loads user config from a file, overriding defaults.
 * Returns true if successful, false if file is missing or has errors.
 */
bool load_config_file(KeyBindings *kb, const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        // No config file found; keep defaults
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Remove trailing newline
        char *p = strchr(line, '\n');
        if (p) *p = '\0';

        // Skip empty lines or comments
        if (line[0] == '#' || strlen(line) == 0)
            continue;

        // Format: key_name=VALUE
        char *eq = strchr(line, '=');
        if (!eq) continue;  // Malformed line, skip

        *eq = '\0';
        char *name = line;
        char *val  = eq + 1;

        // Trim whitespace
        while (isspace((unsigned char)*name)) name++;
        while (isspace((unsigned char)*val))  val++;

        int parsed = parse_key(val);
        if (parsed == -1) continue; // Invalid key, skip

        // Map parsed key to the appropriate struct field
        if      (strcmp(name, "key_up") == 0)        kb->key_up = parsed;
        else if (strcmp(name, "key_down") == 0)      kb->key_down = parsed;
        else if (strcmp(name, "key_left") == 0)      kb->key_left = parsed;
        else if (strcmp(name, "key_right") == 0)     kb->key_right = parsed;
        else if (strcmp(name, "key_tab") == 0)       kb->key_tab = parsed;
        else if (strcmp(name, "key_exit") == 0)      kb->key_exit = parsed;
        else if (strcmp(name, "key_edit") == 0)      kb->key_edit = parsed;
        else if (strcmp(name, "key_copy") == 0)      kb->key_copy = parsed;
        else if (strcmp(name, "key_paste") == 0)     kb->key_paste = parsed;
        else if (strcmp(name, "key_cut") == 0)       kb->key_cut = parsed;
        else if (strcmp(name, "key_delete") == 0)    kb->key_delete = parsed;
        else if (strcmp(name, "key_rename") == 0)    kb->key_rename = parsed;
        else if (strcmp(name, "key_new") == 0)       kb->key_new = parsed;
        else if (strcmp(name, "key_save") == 0)      kb->key_save = parsed;

        // Editing mode keys
        else if (strcmp(name, "edit_up") == 0)        kb->edit_up = parsed;
        else if (strcmp(name, "edit_down") == 0)      kb->edit_down = parsed;
        else if (strcmp(name, "edit_left") == 0)      kb->edit_left = parsed;
        else if (strcmp(name, "edit_right") == 0)     kb->edit_right = parsed;
        else if (strcmp(name, "edit_save") == 0)      kb->edit_save = parsed;
        else if (strcmp(name, "edit_quit") == 0)      kb->edit_quit = parsed;
        else if (strcmp(name, "edit_backspace") == 0) kb->edit_backspace = parsed;
    }

    fclose(fp);
    return true;
}

/**
 * Utility function to parse textual representations of keys:
 *   parse_key("KEY_UP") -> KEY_UP
 *   parse_key("^C")     -> 3
 *   parse_key("F1")     -> KEY_F(1)
 *   parse_key("Tab")    -> '\t'
 *   parse_key("x")      -> 'x'
 *
 * Returns -1 on failure.
 */
static int parse_key(const char *val) {
    // Check for special ncurses keys
    if (strcasecmp(val, "KEY_UP") == 0)        return KEY_UP;
    if (strcasecmp(val, "KEY_DOWN") == 0)      return KEY_DOWN;
    if (strcasecmp(val, "KEY_LEFT") == 0)      return KEY_LEFT;
    if (strcasecmp(val, "KEY_RIGHT") == 0)     return KEY_RIGHT;
    if (strncasecmp(val, "KEY_F(", 6) == 0) {
        // e.g. KEY_F(1), KEY_F(2)
        // naive parse:
        int fnum = atoi(val + 6);
        return KEY_F(fnum);
    }
    if (strcasecmp(val, "KEY_BACKSPACE") == 0 ||
        strcasecmp(val, "Backspace") == 0)
    {
        return KEY_BACKSPACE;
    }
    // F1 / F2 / F3 ...
    if (strcasecmp(val, "F1") == 0) return KEY_F(1);
    if (strcasecmp(val, "F2") == 0) return KEY_F(2);
    if (strcasecmp(val, "F3") == 0) return KEY_F(3);
    // ... up to however many function keys you want

    // Ctrl+X forms, e.g. "^C" is ASCII 3
    if (val[0] == '^' && val[1] != '\0' && val[2] == '\0') {
        // '^C' -> 3
        // '^A' -> 1
        char c = toupper(val[1]);
        if (c >= 'A' && c <= 'Z') {
            return c - 'A' + 1;  // ASCII control code
        }
        return -1; // invalid
    }

    // "Tab"
    if (strcasecmp(val, "Tab") == 0)
        return '\t';

    // Single character?
    if (strlen(val) == 1) {
        return (unsigned char)val[0];
    }

    // Otherwise, unknown
    return -1;
}
