// utils.h

#ifndef UTILS_H
#define UTILS_H

#define EDITOR_COMMAND "nano"  // Change this to your preferred default text editor

[[noreturn]]
void die(int r, const char *format, ...);

void create_file(const char *filename);
void edit_file(const char *filename);
void display_files(const char *directory);
void preview_file(const char *filename);
void change_directory(const char *new_directory, const char ***files, int *num_files, int *selected_entry, int *start_entry, int *end_entry);

#endif