#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <main.h>
#include <utils.h>
#include <files.h>
#include <curses.h>

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
            total_size += get_directory_size(path);
        else
            total_size += statbuf.st_size; // Add size of regular file
    }

    closedir(dir);
    return total_size;
}

char* format_file_size(char *buffer, size_t size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double fileSize = (double)size;
    while (fileSize >= 1024 && i < 4) {
        fileSize /= 1024;
        i++;
    }
    sprintf(buffer, "%.2f %s", fileSize, units[i]);
    return buffer;
}

void display_file_info(WINDOW *window, const char *file_path) {
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
        char fileSizeStr[20];
        mvwprintw(window, 5, 1, "Directory Size: %s", format_file_size(fileSizeStr, dir_size));
    } else {
        // If it's a regular file, display its size directly
        char fileSizeStr[20];
        mvwprintw(window, 5, 1, "File Size: %s", format_file_size(fileSizeStr, file_stat.st_size));
    }

    mvwprintw(window, 6, 1, "File Permissions: %o", file_stat.st_mode & 0777);
    mvwprintw(window, 7, 1, "Last Modification Time: %s", ctime(&file_stat.st_mtime));
}
