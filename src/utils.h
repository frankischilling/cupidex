// utils.h

#ifndef UTILS_H
#define UTILS_H

#define EDITOR_COMMAND "nano"  // Change this to your preferred default text editor

[[noreturn]]
void die(int r, const char *format, ...);

void create_file(const char *filename);
void edit_file(const char *filename);

#endif
