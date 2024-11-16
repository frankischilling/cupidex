// globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include <signal.h>  // For sig_atomic_t

extern volatile sig_atomic_t resized;
extern volatile sig_atomic_t is_editing;

#endif // GLOBALS_H
