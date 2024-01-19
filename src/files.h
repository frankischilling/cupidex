#include <vector.h>

// 256 in most systems
#define MAX_FILENAME_LEN 512

typedef struct FileAttributes* FileAttr;

const char *FileAttr_get_name(FileAttr fa);
void append_files_to_vec(Vector *v, const char *name);

