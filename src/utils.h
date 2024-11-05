// Must be signed
#include <stdbool.h>

#define SIZE int

#define EDITOR_COMMAND "nano"  // Change this to your preferred default text editor

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

__attribute__((noreturn))
void die(int r, const char *format, ...);

void path_join(char *result, const char *path1, const char *path2);
void create_file(const char *filename);
void display_files(const char *directory);
void preview_file(const char *filename);
void change_directory(const char *new_directory, const char ***files, int *num_files, int *selected_entry, int *start_entry, int *end_entry);
bool is_directory(const char *path, const char *filename);
