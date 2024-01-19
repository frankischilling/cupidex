#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include <utils.h>
#include "files.h"

struct FileAttributes {
    char name[MAX_FILENAME_LEN];
    // According to SUSv4 ino_t shall be defined as an unsigned integer type,
    // SUSv2 says "extended unsigned integral type". It's most likely valid to
    // copy it.
    ino_t inode;
    bool is_dir;
};

const char *FileAttr_get_name(FileAttr fa) {
    return fa->name;
}

FileAttr mk_attr(const char *name, bool is_dir, ino_t inode) {
    FileAttr fa = malloc(sizeof(struct FileAttributes));

    size_t namelen = strlen(name);
    memcpy(fa->name, name, MIN(namelen + 1, MAX_FILENAME_LEN));

    fa->inode = inode;
    fa->is_dir = is_dir;

    return fa;
}

void append_files_to_vec(Vector *v, const char *name) {
    DIR *dir = opendir(name);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Filter out "." and ".." entries
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                Vector_add(v, 1);
                v->el[Vector_len(*v)] = mk_attr(
                    entry->d_name, false, entry->d_ino
                );
                Vector_set_len(v, Vector_len(*v) + 1);
            }
        }
        closedir(dir);
    }
}

