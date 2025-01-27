// config.c
#include "config.h"
#include <strings.h>    // For string manipulation functions (e.g., strcasecmp)
#include <string.h>     // For standard string functions (e.g., strlen, strchr)
#include <stdio.h>      // For file I/O operations (e.g., fopen, fgets, snprintf)
#include <stdlib.h>     // For general utilities (e.g., atoi)
#include <ctype.h>      // For character handling functions (e.g., isspace, toupper)
#include <ncurses.h>    // For ncurses library functions and key definitions

/**
 * @brief Trims leading whitespace from a string.
 *
 * This function modifies the input string pointer to skip all leading whitespace characters.
 *
 * @param s The input string to trim.
 * @return A pointer to the first non-whitespace character in the string.
 */
static char* ltrim(char *s) {
    while (isspace((unsigned char)*s)) s++;  // Increment pointer until a non-space character is found
    return s;
}

/**
 * @brief Trims trailing whitespace from a string.
 *
 * This function modifies the input string in place by inserting a null terminator
 * after the last non-whitespace character, effectively removing trailing spaces.
 *
 * @param s The input string to trim.
 */
static void rtrim(char *s) {
    char *back = s + strlen(s);  // Pointer to the end of the string
    while (back > s && isspace((unsigned char)*(back - 1))) back--;  // Move back to skip trailing spaces
    *back = '\0';  // Insert null terminator to trim the string
}

/**
 * @brief Trims both leading and trailing whitespace from a string.
 *
 * This function applies `ltrim` and `rtrim` to remove whitespace from both ends of the string.
 *
 * @param s The input string to trim.
 * @return A pointer to the trimmed string.
 */
static char* trim(char *s) {
   rtrim(s);
   return ltrim(s);
}

/**
 * @brief Parses the key value from its textual representation.
 *
 * This function interprets various string formats representing keys (e.g., "KEY_UP", "^C", "F1")
 * and converts them to their corresponding integer key codes.
 *
 * @param val The string representation of the key.
 * @return The corresponding key code, or -1 if parsing fails.
 */
static int parse_key(const char *val);

/**
 * @brief Loads default keybindings into the provided KeyBindings structure.
 *
 * This function initializes all keybinding fields with default values using predefined
 * ncurses key constants and ASCII control codes.
 *
 * @param kb Pointer to the KeyBindings structure to initialize.
 */
void load_default_keybindings(KeyBindings *kb) {
    // Arrow keys using ncurses constants
    kb->key_up      = KEY_UP;
    kb->key_down    = KEY_DOWN;
    kb->key_left    = KEY_LEFT;
    kb->key_right   = KEY_RIGHT;
    
    // Special keys
    kb->key_tab     = '\t';       // Tab key
    kb->key_exit    = KEY_F(1);   // F1 key for exit
    
    // Control keys using ASCII control codes
    kb->key_edit    = 5;    // Ctrl+E (ASCII 5)
    kb->key_copy    = 3;    // Ctrl+C (ASCII 3)
    kb->key_paste   = 22;   // Ctrl+V (ASCII 22)
    kb->key_cut     = 24;   // Ctrl+X (ASCII 24)
    kb->key_delete  = 4;    // Ctrl+D (ASCII 4)
    kb->key_rename  = 18;   // Ctrl+R (ASCII 18)
    kb->key_new     = 14;   // Ctrl+N (ASCII 14)
    kb->key_save    = 19;   // Ctrl+S (ASCII 19)
    kb->key_new_dir = 14;   // Ctrl+N reused for creating new directories
    
    // Dedicated editing mode keys
    kb->edit_up        = KEY_UP;
    kb->edit_down      = KEY_DOWN;
    kb->edit_left      = KEY_LEFT;
    kb->edit_right     = KEY_RIGHT;
    kb->edit_save      = 19;  // Ctrl+S for saving in edit mode
    kb->edit_quit      = 17;  // Ctrl+Q for quitting edit mode
    kb->edit_backspace = KEY_BACKSPACE; // Backspace key
}

/**
 * @brief Loads user configuration from a file, overriding defaults.
 *
 * This function reads a configuration file line by line, parses key-value pairs,
 * and updates the KeyBindings structure accordingly. It handles comments, whitespace,
 * and reports any parsing errors encountered.
 *
 * @param kb Pointer to the KeyBindings structure to update.
 * @param filepath Path to the configuration file.
 * @param error_buffer Buffer to store error messages.
 * @param buffer_size Size of the error_buffer.
 * @return The number of errors encountered during loading. 0 indicates success.
 */
int load_config_file(KeyBindings *kb, const char *filepath, char *error_buffer, size_t buffer_size) {
    FILE *fp = fopen(filepath, "r");  // Open the configuration file for reading
    if (!fp) {
        // If the file cannot be opened, retain default keybindings and report the issue
        snprintf(error_buffer, buffer_size, "Configuration file not found. Using default settings.\n");
        return 1; // Indicate one "non-fatal" error
    }

    char line[256];               // Buffer to store each line from the file
    size_t error_count = 0;       // Counter for the number of errors encountered
    size_t line_number = 0;       // To track the current line number for error reporting

    // Read the file line by line
    while (fgets(line, sizeof(line), fp)) {
        line_number++;

        // Remove trailing newline character, if present
        char *p = strchr(line, '\n');
        if (p) *p = '\0';

        // Remove inline comments by truncating at the first '#'
        p = strchr(line, '#');
        if (p) *p = '\0';

        // Trim leading and trailing whitespace after removing comments
        char *trimmed_line = trim(line);

        // Skip empty lines after trimming
        if (strlen(trimmed_line) == 0)
            continue;

        // Expected format: key_name=VALUE
        char *eq = strchr(trimmed_line, '=');
        if (!eq) {
            // Malformed line without '='
            if (error_count < buffer_size - 1) {
                snprintf(error_buffer + strlen(error_buffer), buffer_size - strlen(error_buffer) - 1,
                         "Line %zu: Malformed line (no '='): %s\n", line_number, trimmed_line);
            }
            error_count++;
            continue;
        }

        *eq = '\0';  // Split the line into key and value
        char *name = trimmed_line;
        char *val  = eq + 1;

        // Trim whitespace from both key and value
        name = trim(name);
        val = trim(val);

        // Skip processing if key or value is empty after trimming
        if (strlen(name) == 0 || strlen(val) == 0) {
            if (error_count < buffer_size - 1) {
                snprintf(error_buffer + strlen(error_buffer), buffer_size - strlen(error_buffer) - 1,
                         "Line %zu: Malformed line (empty key or value): %s=%s\n", line_number, name, val);
            }
            error_count++;
            continue;
        }

        // Parse the value to obtain the corresponding key code
        int parsed = parse_key(val);
        if (parsed == -1) {
            // Invalid key value encountered
            if (error_count < buffer_size - 1) {
                snprintf(error_buffer + strlen(error_buffer), buffer_size - strlen(error_buffer) - 1,
                         "Line %zu: Invalid key value for '%s': %s\n", line_number, name, val);
            }
            error_count++;
            continue;
        }

        // Map the parsed key code to the appropriate field in the KeyBindings structure
        if      (strcasecmp(name, "key_up") == 0)        kb->key_up = parsed;
        else if (strcasecmp(name, "key_down") == 0)      kb->key_down = parsed;
        else if (strcasecmp(name, "key_left") == 0)      kb->key_left = parsed;
        else if (strcasecmp(name, "key_right") == 0)     kb->key_right = parsed;
        else if (strcasecmp(name, "key_tab") == 0)       kb->key_tab = parsed;
        else if (strcasecmp(name, "key_exit") == 0)      kb->key_exit = parsed;
        else if (strcasecmp(name, "key_edit") == 0)      kb->key_edit = parsed;
        else if (strcasecmp(name, "key_copy") == 0)      kb->key_copy = parsed;
        else if (strcasecmp(name, "key_paste") == 0)     kb->key_paste = parsed;
        else if (strcasecmp(name, "key_cut") == 0)       kb->key_cut = parsed;
        else if (strcasecmp(name, "key_delete") == 0)    kb->key_delete = parsed;
        else if (strcasecmp(name, "key_rename") == 0)    kb->key_rename = parsed;
        else if (strcasecmp(name, "key_new") == 0)       kb->key_new = parsed;
        else if (strcasecmp(name, "key_save") == 0)      kb->key_save = parsed;
        else if (strcasecmp(name, "key_new_dir") == 0)   kb->key_new_dir = parsed;
        // Editing mode keys
        else if (strcasecmp(name, "edit_up") == 0)        kb->edit_up = parsed;
        else if (strcasecmp(name, "edit_down") == 0)      kb->edit_down = parsed;
        else if (strcasecmp(name, "edit_left") == 0)      kb->edit_left = parsed;
        else if (strcasecmp(name, "edit_right") == 0)     kb->edit_right = parsed;
        else if (strcasecmp(name, "edit_save") == 0)      kb->edit_save = parsed;
        else if (strcasecmp(name, "edit_quit") == 0)      kb->edit_quit = parsed;
        else if (strcasecmp(name, "edit_backspace") == 0) kb->edit_backspace = parsed;
        else {
            // Unknown configuration key encountered
            if (error_count < buffer_size - 1) {
                snprintf(error_buffer + strlen(error_buffer), buffer_size - strlen(error_buffer) - 1,
                         "Line %zu: Unknown configuration key: %s\n", line_number, name);
            }
            error_count++;
            continue;
        }
    }

    fclose(fp);  // Close the configuration file
    return error_count;  // Return the total number of errors encountered
}

/**
 * @brief Utility function to parse textual representations of keys.
 *
 * This function interprets various string formats representing keys and converts them
 * to their corresponding integer key codes. Supported formats include:
 *   - Special ncurses keys (e.g., "KEY_UP")
 *   - Function keys (e.g., "F1", "KEY_F(1)")
 *   - Control characters (e.g., "^C")
 *   - Named keys (e.g., "Tab", "Space")
 *   - Shift-modified keys (e.g., "Shift+A")
 *   - Single character keys (e.g., "x")
 *
 * @param val The string representation of the key.
 * @return The corresponding key code, or -1 if parsing fails.
 */
static int parse_key(const char *val) {
    // Check for special ncurses keys
    if (strcasecmp(val, "KEY_UP") == 0)        return KEY_UP;
    if (strcasecmp(val, "KEY_DOWN") == 0)      return KEY_DOWN;
    if (strcasecmp(val, "KEY_LEFT") == 0)      return KEY_LEFT;
    if (strcasecmp(val, "KEY_RIGHT") == 0)     return KEY_RIGHT;

    // Handle keys in the format "KEY_F(n)" where n is a number
    if (strncasecmp(val, "KEY_F(", 6) == 0) {
        // Extract the number inside "KEY_F(n)"
        int fnum = atoi(val + 6);
        if (fnum >= 1 && fnum <= 63) { // ncurses typically supports F1-F63
            return KEY_F(fnum);
        }
        return -1; // Invalid function key number
    }

    // Check for backspace keys
    if (strcasecmp(val, "KEY_BACKSPACE") == 0 ||
        strcasecmp(val, "Backspace") == 0)
    {
        return KEY_BACKSPACE;
    }

    // Support for function keys F1 through F12
    for (int i = 1; i <= 12; i++) {
        char fn_key[5];
        snprintf(fn_key, sizeof(fn_key), "F%d", i);
        if (strcasecmp(val, fn_key) == 0) {
            return KEY_F(i);
        }
    }

    // Handle Control+X keys, e.g., "^C" represents Ctrl+C
    if (val[0] == '^' && val[1] != '\0' && val[2] == '\0') {
        char c = toupper(val[1]);
        if (c >= 'A' && c <= 'Z') {
            return c - 'A' + 1;  // Calculate ASCII control code
        }
        return -1; // Invalid control key format
    }

    // Support named keys like "Tab" and "Space"
    if (strcasecmp(val, "Tab") == 0) return '\t';
    if (strcasecmp(val, "Space") == 0) return ' ';

    // Handle Shift-modified keys, e.g., "Shift+A" represents the 'A' key with Shift
    if (strncasecmp(val, "Shift+", 6) == 0) {
        char shift_key = val[6];

        // Convert alphabetic keys to uppercase
        if (isalpha(shift_key)) {
            return toupper(shift_key);
        }

        // Handle Shift-modified number keys by mapping to corresponding symbols
        static const char shift_symbols[] = ")!@#$%^&*(";
        if (isdigit(shift_key)) {
            int num = shift_key - '0';
            if (num >= 0 && num <= 9) {
                return shift_symbols[num]; // Returns corresponding symbol for Shift+number
            }
        }

        // Handle common Shift-symbol keys by their names
        if (strcmp(val + 6, "Minus") == 0) return '_';
        if (strcmp(val + 6, "Equals") == 0) return '+';
        if (strcmp(val + 6, "LeftBracket") == 0) return '{';
        if (strcmp(val + 6, "RightBracket") == 0) return '}';
        if (strcmp(val + 6, "Semicolon") == 0) return ':';
        if (strcmp(val + 6, "Apostrophe") == 0) return '"';
        if (strcmp(val + 6, "Comma") == 0) return '<';
        if (strcmp(val + 6, "Period") == 0) return '>';
        if (strcmp(val + 6, "Slash") == 0) return '?';
        if (strcmp(val + 6, "Backslash") == 0) return '|';
        if (strcmp(val + 6, "Grave") == 0) return '~';

        return -1; // Invalid Shift-modified key
    }

    // Handle single character keys (non-shifted)
    if (strlen(val) == 1) {
        return (unsigned char)val[0];
    }

    return -1; // Unknown key format
}
