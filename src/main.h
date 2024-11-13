//
// Created by frank on 1/22/24.
//

#ifndef CUPIDFM_MAIN_H
#define CUPIDFM_MAIN_H

#include <curses.h>    // Add this for WINDOW type
#include <time.h>      // Add this for struct timespec

// Global variables
extern const char *BANNER_TEXT;
extern const char *BUILD_INFO;
extern WINDOW *bannerwin;
extern WINDOW *notifwin;
extern struct timespec last_scroll_time;

// Function declarations
void draw_scrolling_banner(WINDOW *window, const char *text, const char *build_info, int offset);

#define BANNER_UPDATE_INTERVAL 50000  // 50ms in microseconds

#endif //CUPIDFM_MAIN_H
