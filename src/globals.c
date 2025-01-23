#include <curses.h>    // Ncurses library for terminal-based UI
#include <pthread.h>   // For multi-threading support
#include "globals.h"   // Global variable declarations

/**
 * @brief Stores the banner text to be displayed in the UI.
 *
 * This variable is initialized to `NULL` and should be set in `main()`
 * before use. It contains the scrolling banner message.
 */
const char *BANNER_TEXT = NULL;

/**
 * @brief Stores build version information.
 *
 * This string holds the version number of the application.
 * It is displayed in the UI banner.
 */
const char *BUILD_INFO = "Version 1.0";

/**
 * @brief ncurses window pointer for the scrolling banner.
 *
 * This window is used to display the top banner text in the UI.
 * It should be initialized properly before being used.
 */
WINDOW *bannerwin = NULL;

/**
 * @brief Ncurses window pointer for notifications.
 *
 * This window displays user notifications or system messages.
 */
WINDOW *notifwin = NULL;

/**
 * @brief Tracks the last scroll time for the banner.
 *
 * This struct stores the timestamp (seconds and nanoseconds) of the last
 * banner scroll event. It is used to regulate the scroll speed.
 */
struct timespec last_scroll_time = {0, 0};

/**
 * @brief Mutex to synchronize access to the banner text.
 *
 * Since the banner text might be updated by multiple threads,
 * this mutex ensures thread-safe access and modifications.
 */
pthread_mutex_t banner_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Controls whether the notification window should be cleared automatically.
 *
 * If `true`, the notification window will be cleared after a timeout.
 */
bool should_clear_notif = true;

/**
 * @brief Tracks the last time a notification was displayed.
 *
 * This struct stores the timestamp (seconds and nanoseconds) of the last notification
 * update. It is used to determine when to clear or update notifications.
 */
struct timespec last_notification_time = {0, 0};
