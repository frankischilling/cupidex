#ifndef CUPIDFM_MAIN_H
#define CUPIDFM_MAIN_H

#include <curses.h>    // For WINDOW type
#include <time.h>      // For struct timespec
#include <pthread.h>   // For mutex
#include <signal.h>    // For sig_atomic_t
#include "globals.h"

// ─────────────────────────────────────────────────────────────
// Global Variables
// ─────────────────────────────────────────────────────────────

extern const char *BANNER_TEXT;  // The main text displayed in the banner
extern const char *BUILD_INFO;   // Additional build information shown in the banner
extern WINDOW *bannerwin;        // Pointer to the banner window
extern WINDOW *notifwin;         // Pointer to the notification window
extern struct timespec last_scroll_time;  // Stores the last time the banner was updated

// ─────────────────────────────────────────────────────────────
// Synchronization Variables
// ─────────────────────────────────────────────────────────────

extern pthread_mutex_t banner_mutex;  // Mutex for synchronizing banner updates
extern bool banner_running;           // Flag to indicate if the banner is running
extern volatile sig_atomic_t running; // Flag to indicate if the application is running

// ─────────────────────────────────────────────────────────────
// Function Declarations
// ─────────────────────────────────────────────────────────────

/**
 * Draws a scrolling banner in the specified window.
 * @param window The window where the banner is displayed.
 * @param text The main text of the banner.
 * @param build_info Additional build information to display.
 * @param offset The current offset for scrolling.
 */
void draw_scrolling_banner(WINDOW *window, const char *text, const char *build_info, int offset);

/**
 * Handles the scrolling of the banner text in a separate thread.
 * @param arg A pointer to the window where the banner is displayed.
 * @return A pointer to the thread's return value.
 */
void *banner_scrolling_thread(void *arg);

/**
 * Displays a notification message in the specified window.
 * @param win The window to display the notification in.
 * @param format A formatted string containing the notification message.
 */
void show_notification(WINDOW *win, const char *format, ...);

// ─────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────

#define BANNER_UPDATE_INTERVAL 250000  // Banner update interval (250ms) in microseconds

#endif // CUPIDFM_MAIN_H
