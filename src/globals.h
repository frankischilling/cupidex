// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#define MAX_PATH_LENGTH 1024  // Define it here consistently

#include <signal.h>  // For sig_atomic_t

extern volatile sig_atomic_t resized;
extern volatile sig_atomic_t is_editing;
extern char copied_filename[MAX_PATH_LENGTH];

#endif // GLOBALS_H
