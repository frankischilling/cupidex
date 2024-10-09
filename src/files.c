// File: files.c
// -----------------------
#define _POSIX_C_SOURCE 200112L    // for strdup
#include <stdlib.h>                // for malloc, free
#include <stddef.h>                // for NULL
#include <sys/types.h>             // for ino_t
#include <string.h>                // for strdup
#include <dirent.h>                // for DIR, struct dirent, opendir, readdir, closedir
#include <unistd.h>                // for lstat
#include <stdio.h>                 // for snprintf
#include <sys/stat.h>              // for struct stat, lstat, S_ISDIR
#include <time.h>                  // for strftime
#include <main.h>                  // for FileAttr, Vector, Vector_add, Vector_len, Vector_set_len
#include <utils.h>                 // for path_join, is_directory
#include <files.h>                 // for FileAttributes, FileAttr, MAX_PATH_LENGTH
#include <curses.h>                // for WINDOW, mvwprintw
#include <stdbool.h>               // for bool, true, false
#include <string.h>                // for strcmp

#define MAX_PATH_LENGTH 1024

struct FileAttributes {
    char *name;  // Change from char name*;
    ino_t inode;
    bool is_dir;
};

const char *FileAttr_get_name(FileAttr fa) {
    if (fa != NULL) {
        return fa->name;
    } else {
        // Handle the case where fa is NULL
        return "Unknown";
    }
}


bool FileAttr_is_dir(FileAttr fa) {
    return fa->is_dir;
}

FileAttr mk_attr(const char *name, bool is_dir, ino_t inode) {
    FileAttr fa = malloc(sizeof(struct FileAttributes));

    if (fa != NULL) {
        fa->name = strdup(name);

        if (fa->name == NULL) {
            // Handle memory allocation failure for the name
            free(fa);
            return NULL;
        }

        fa->inode = inode;
        fa->is_dir = is_dir;
        return fa;
    } else {
        // Handle memory allocation failure for the FileAttr
        return NULL;
    }
}

void free_attr(FileAttr fa) {
    if (fa != NULL) {
        free(fa->name);  // Free the allocated memory for the name
        free(fa);
    }
}

void append_files_to_vec(Vector *v, const char *name) {
    DIR *dir = opendir(name);
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Filter out "." and ".." entries
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char full_path[MAX_PATH_LENGTH];
                path_join(full_path, name, entry->d_name);

                bool is_dir = is_directory(name, entry->d_name);

                // Allocate memory for the FileAttr object
                FileAttr file_attr = mk_attr(entry->d_name, is_dir, entry->d_ino);

                // Add the FileAttr object to the vector
                Vector_add(v, 1);
                v->el[Vector_len(*v)] = file_attr;

                // Update the vector length
                Vector_set_len(v, Vector_len(*v) + 1);
            }
        }
        closedir(dir);
    }
}

// Recursive function to calculate directory size
// NOTE: this function may take long, it might be better to have the size of
//       the directories displayed as "-" until we have a value. Use of "du" or
//       another already existent tool might be better. The sizes should
//       probably be cached.
//       fork() -> exec() (if using du)
//       fork() -> calculate directory size and return it somehow (maybe print
//       as binary to the stdout)
long get_directory_size(const char *dir_path) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    long total_size = 0;

    // Open directory
    if (!(dir = opendir(dir_path)))
        return -1;

    // Iterate over directory entries
    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATH_LENGTH];
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        // Construct full path to entry
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        // Get entry's information
        if (lstat(path, &statbuf) == -1)
            continue;
        // If entry is a directory, recursively calculate its size
        if (S_ISDIR(statbuf.st_mode))
            // This recursion may cause a stack overflow
            total_size += get_directory_size(path);
        else
            total_size += statbuf.st_size; // Add size of regular file
    }

    closedir(dir);
    return total_size;
}

char* format_file_size(char *buffer, size_t size) {
    // iB for multiples of 1024, B for multiples of 1000
    // so, KiB = 1024, KB = 1000
    const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    int i = 0;
    double fileSize = (double)size;
    while (fileSize >= 1024 && i < 4) {
        fileSize /= 1024;
        i++;
    }
    sprintf(buffer, "%.2f %s", fileSize, units[i]);
    return buffer;
}

void display_file_info(WINDOW *window, const char *file_path, int max_x) {
    struct stat file_stat;

    // Get file information
    if (stat(file_path, &file_stat) == -1) {
        mvwprintw(window, 5, 1, "Unable to retrieve file information");
        return;
    }

    // Display file information
    if (S_ISDIR(file_stat.st_mode)) {
        // If it's a directory, calculate its size using get_directory_size
        long dir_size = get_directory_size(file_path);
        // Check if this array is big enough, greater than needed is better
        // than smaller
        char fileSizeStr[20];
        mvwprintw(window, 5, 2, "Directory Size: %.*s", max_x - 4, format_file_size(fileSizeStr, dir_size));
    } else {
        // If it's a regular file, display its size directly
        char fileSizeStr[20];
        mvwprintw(window, 5, 2, "File Size: %.*s", max_x - 4, format_file_size(fileSizeStr, file_stat.st_size));
    }

    // Check if this array is big enough for the number
    // My check (needs review): 18 (label) +
    char permissions[22];
    sprintf(permissions, "File Permissions: %o", file_stat.st_mode & 0777);
    mvwprintw(window, 6, 2, "%.*s", max_x - 4, permissions);

    char modTime[50];
    strftime(modTime, sizeof(modTime), "%c", localtime(&file_stat.st_mtime));
    mvwprintw(window, 7, 2, "Last Modification Time: %.24s", modTime);
}

/**
 * Checks if the given file has a supported extension.
 *
 * @param filename The name of the file to check.
 * @return true if the file has a supported extension, false otherwise.
 */
bool is_supported_file_type(const char *filename) {
    const char *extensions[] = {
        ".txt", ".c", ".java", ".py", ".cpp", ".h", ".hpp", ".js", ".html", ".css",
        ".md", ".json", ".xml", ".sh", ".bat", ".ini", ".conf", ".log", ".csv", ".tsv",
        ".yaml", ".yml", ".toml", ".rb", ".php", ".pl", ".rs", ".go", ".swift", ".kt"
    };
    size_t num_extensions = sizeof(extensions) / sizeof(extensions[0]);
    const char *ext = strrchr(filename, '.');
    if (ext) {
        for (size_t i = 0; i < num_extensions; i++) {
            if (strcmp(ext, extensions[i]) == 0) {
                return true;
            }
        }
    }
    return false;
}