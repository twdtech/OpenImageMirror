#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <uuid/uuid.h>
#include <openssl/md5.h>

#include "utils.h"

int create_directory(const char *path) {
    return mkdir(path, 0755);
}

bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

bool is_directory(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

long get_file_size(const char *path) {
    struct stat statbuf;
    return (stat(path, &statbuf) == 0) ? statbuf.st_size : -1;
}

char* str_duplicate(const char *src) {
    if (src == NULL) return NULL;
    return strdup(src);
}

char* str_trim(char *str) {
    if (str == NULL) return NULL;

    while (isspace((unsigned char)*str)) str++;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    return str;
}

char* str_to_lowercase(char *str) {
    if (str == NULL) return NULL;

    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
    return str;
}

bool str_ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) return false;

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    return str_len >= suffix_len && 
           strcmp(str + str_len - suffix_len, suffix) == 0;
}

char* generate_uuid() {
    uuid_t uuid;
    char *uuid_str = malloc(37);  

    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, uuid_str);

    return uuid_str;
}

char* path_join(const char *path1, const char *path2) {
    if (!path1) return str_duplicate(path2);
    if (!path2) return str_duplicate(path1);

    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);

    while (len1 > 0 && path1[len1-1] == '/') len1--;

    while (len2 > 0 && path2[0] == '/') {
        path2++;
        len2--;
    }

    char *result = malloc(len1 + len2 + 2);  
    sprintf(result, "%.*s/%s", (int)len1, path1, path2);

    return result;
}

char* get_current_timestamp_str() {
    time_t now;
    time(&now);
    return str_duplicate(ctime(&now));
}

long get_available_disk_space(const char *path) {
    struct statvfs stat;
    if (statvfs(path, &stat) != 0) {
        return -1;
    }
    return stat.f_frsize * stat.f_bavail;
}

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

long get_total_memory() {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}