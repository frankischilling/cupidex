#ifndef GLOBALS_H
#define GLOBALS_H

#include <signal.h>   // For sig_atomic_t (safe for signal handlers)
#include <time.h>     // For struct timespec (timing operations)
#include <stdbool.h>  // For boolean type (true/false)

// ─────────────────────────────────────────────────────────────
// Global Configuration Constants
// ─────────────────────────────────────────────────────────────

/**
 * @brief Defines the maximum length for file paths.
 *
 * This constant ensures that path-related buffers have a consistent size
 * across the application, preventing buffer overflows.
 */
#define MAX_PATH_LENGTH 1024  

/**
 * @brief Timeout duration for notifications in milliseconds.
 *
 * Notifications will automatically clear after this period.
 */
#define NOTIFICATION_TIMEOUT_MS 250  // 250ms timeout for notifications

/**
 * @brief Defines the maximum length for directory names.
 *
 * This value ensures uniform buffer sizes when handling directory names.
 */
#define MAX_DIR_NAME 256

/**
 * @brief Maximum number of characters to display in truncated file/directory names.
 *
 * If a name exceeds this length, it may be shortened for display purposes.
 */
#define MAX_DISPLAY_LENGTH 32

/**
 * @brief ASCII value for the Tab key.
 *
 * This is commonly used for navigation in text-based applications.
 */
#define TAB 9

/**
 * @brief ASCII value for the Ctrl+E key combination.
 *
 * Used for editing shortcuts in the application.
 */
#define CTRL_E 5

/**
 * @brief Interval for scrolling the banner text, in microseconds.
 *
 * This determines the delay between each scroll update in the banner display.
 */
#define BANNER_SCROLL_INTERVAL 250000  // 250ms in microseconds

/**
 * @brief Interval for checking user input, in milliseconds.
 *
 * This determines how frequently the program checks for keyboard input.
 */
#define INPUT_CHECK_INTERVAL 10  // 10ms interval

/**
 * @brief Defines the maximum size of the error message buffer.
 *
 * Ensures sufficient space for storing detailed error messages.
 */
#define ERROR_BUFFER_SIZE 2048  // Increased buffer size for detailed errors

/**
 * ========================== Global Variables (Extern) ==========================
 */

/**
 * @brief Tracks whether the terminal has been resized.
 *
 * This volatile variable is updated asynchronously via a signal handler.
 */
extern volatile sig_atomic_t resized;

/**
 * @brief Indicates whether the user is in edit mode.
 *
 * This flag determines whether editing operations should be active.
 */
extern volatile sig_atomic_t is_editing;

/**
 * @brief Stores the most recently copied filename.
 *
 * This buffer is used when handling file copy-paste operations.
 */
extern char copied_filename[MAX_PATH_LENGTH];

/**
 * @brief Stores the timestamp of the last notification.
 *
 * This is used to manage notification timeouts.
 */
extern struct timespec last_notification_time;

/**
 * @brief Controls whether the notification window should be cleared automatically.
 *
 * If set to `true`, notifications will be cleared after a timeout.
 */
extern bool should_clear_notif;

#endif // GLOBALS_H
