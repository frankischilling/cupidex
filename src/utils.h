// Must be signed
#define SIZE int

#define EDITOR_COMMAND "nano"  // Change this to your preferred default text editor

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) > (y)) ? (y) : (x))

[[noreturn]]
void die(int r, const char *format, ...);

void create_file(const char *filename);
void edit_file(const char *filename);
void display_files(const char *directory);
void preview_file(const char *filename);
void change_directory(const char *new_directory, const char ***files, int *num_files, int *selected_entry, int *start_entry, int *end_entry);
bool is_directory(const char *path, const char *filename);
