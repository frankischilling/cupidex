// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#define MAX_PATH_LENGTH 1024  // Define it here consistently
#define NOTIFICATION_TIMEOUT_MS 250  // 1 second timeout for notifications

#include <signal.h>  // For sig_atomic_t
#include <time.h>
#include <stdbool.h>

extern volatile sig_atomic_t resized;
extern volatile sig_atomic_t is_editing;
extern char copied_filename[MAX_PATH_LENGTH];
extern struct timespec last_notification_time;
extern bool should_clear_notif;

#endif // GLOBALS_H
