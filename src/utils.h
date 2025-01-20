#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <ncurses.h>

#define SIZE int

#define EDITOR_COMMAND "nano"  // Change this to your preferred default text editor

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

__attribute__((noreturn))
void die(int r, const char *format, ...);
bool is_directory(const char *path, const char *filename);

void create_file(const char *filename);
void display_files(const char *directory);
void preview_file(const char *filename);
void change_directory(const char *new_directory, const char ***files, int *num_files, int *selected_entry, int *start_entry, int *end_entry);
void path_join(char *result, const char *base, const char *extra);
const char* get_file_emoji(const char *mime_type, const char *filename);

// short cut utils
void copy_to_clipboard(const char *path);
void paste_from_clipboard(const char *target_directory, const char *filename);
void cut_and_paste(const char *path);
void delete_item(const char *path);
void confirm_delete(WINDOW *notifwin, const char *path, bool *should_delete);
void rename_item(WINDOW *notifwin, const char *old_path);
void create_new_file(WINDOW *notifwin, const char *current_dir);

#endif // UTILS_H