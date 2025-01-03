#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <json-c/json.h>
#include <errno.h>

#include "imgMgr.h"
#include "config.h"
#include "cache.h"
#include "logging.h"

static OIMMirrorManagerConfig *manager_config = NULL;
static json_object *cached_mirror_list = NULL;
static time_t last_scan_time = 0;

int oim_init_mirror_manager(OIMConfig *config) {

    if (config == NULL) {
        LOG_ERROR("Configuration is NULL when initializing Mirror manager");
        return -1;
    }

    LOG_INFO("Initializing Mirror Manager");
    LOG_INFO("Mirror Directory: %s", config->mirror_directory);
    LOG_INFO("Recursive Scan: %d", config->recursive_scan);

    if (config->mirror_directory == NULL || strlen(config->mirror_directory) == 0) {
        LOG_ERROR("Mirror directory path is not set");
        return -1;
    }

    DIR *dir = opendir(config->mirror_directory);
    if (dir == NULL) {
        LOG_ERROR("Cannot open Mirror directory: %s (Error: %s)", 
                  config->mirror_directory, strerror(errno));
        return -1;
    }
    closedir(dir);

    manager_config = malloc(sizeof(OIMMirrorManagerConfig));
    if (manager_config == NULL) {
        LOG_ERROR("Failed to allocate Mirror manager configuration");
        return -1;
    }

    manager_config->base_directory = strdup(config->mirror_directory);
    manager_config->recursive_scan = config->recursive_scan;
    manager_config->scan_interval = config->scan_interval;

    LOG_INFO("Mirror Manager initialized successfully");

    return oim_rescan_mirror_directory();
}

void oim_cleanup_mirror_manager() {
    if (manager_config) {
        free(manager_config->base_directory);
        free(manager_config);
        manager_config = NULL;
    }

    if (cached_mirror_list) {
        json_object_put(cached_mirror_list);
        cached_mirror_list = NULL;
    }
}

int oim_rescan_mirror_directory() {
    time_t current_time = time(NULL);

    LOG_INFO("Rescan Configuration:");
    LOG_INFO("Base Directory: %s", 
             manager_config ? manager_config->base_directory : "NULL");
    LOG_INFO("Recursive Scan: %s", 
             manager_config && manager_config->recursive_scan ? "Yes" : "No");

    if (manager_config == NULL) {
        LOG_ERROR("Mirror Manager Configuration is NULL");
        return -1;
    }

    if (manager_config->base_directory == NULL) {
        LOG_ERROR("Base Mirror directory is not set");
        return -1;
    }

    DIR *dir = opendir(manager_config->base_directory);
    if (dir == NULL) {
        LOG_ERROR("Cannot open Mirror directory: %s (Error: %s)", 
                  manager_config->base_directory, strerror(errno));
        return -1;
    }
    closedir(dir);

    if (current_time - last_scan_time < manager_config->scan_interval && cached_mirror_list) {
        LOG_INFO("Using existing cached Mirror list");
        return 0;
    }

    json_object *mirror_list = json_object_new_array();
    if (mirror_list == NULL) {
        LOG_ERROR("Failed to create Mirror list array");
        return -1;
    }

    int result = oim_scan_directory(
        manager_config->base_directory, 
        manager_config->base_directory, 
        mirror_list
    );

    if (result != 0) {
        LOG_ERROR("Directory scanning failed");
        json_object_put(mirror_list);
        return -1;
    }

    LOG_INFO("Found %d Mirror files", json_object_array_length(mirror_list));

    if (cached_mirror_list) {
        json_object_put(cached_mirror_list);
    }
    cached_mirror_list = mirror_list;
    last_scan_time = current_time;

    oim_cache_store_mirror_list(cached_mirror_list);

    return 0;
}

int oim_scan_directory(
    const char *directory, 
    const char *base_directory, 
    json_object *mirror_list
) {
    DIR *dir;
    struct dirent *entry;
    char full_path[PATH_MAX];
    struct stat file_stat;

    dir = opendir(directory);
    if (dir == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", directory);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        if (stat(full_path, &file_stat) == -1) {
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            if (manager_config->recursive_scan) {
                oim_scan_directory(full_path, base_directory, mirror_list);
            }
            continue;
        }

        const char *ext = strrchr(entry->d_name, '.');
        if (ext && (
            strcasecmp(ext, ".iso") == 0 || 
            strcasecmp(ext, ".img") == 0
        )) {

            json_object *mirror_entry = json_object_new_object();

            char *category = oim_generate_category_from_path(full_path, base_directory);

            json_object_object_add(mirror_entry, "filename", 
                json_object_new_string(entry->d_name));
            json_object_object_add(mirror_entry, "path", 
                json_object_new_string(full_path));
            json_object_object_add(mirror_entry, "category", 
                json_object_new_string(category ? category : "Uncategorized"));
            json_object_object_add(mirror_entry, "size", 
                json_object_new_int64(file_stat.st_size));
            json_object_object_add(mirror_entry, "modified", 
                json_object_new_int64(file_stat.st_mtime));

            json_object_array_add(mirror_list, mirror_entry);

            free(category);
        }
    }

    closedir(dir);
    return 0;
}

char* oim_generate_category_from_path(const char *full_path, const char *base_dir) {

    const char *relative_path = full_path + strlen(base_dir);

    if (relative_path[0] == '/') {
        relative_path++;
    }

    char *category = strdup(relative_path);
    char *slash = strchr(category, '/');

    if (slash) {
        *slash = '\0';
    }

    return category;
}

json_object* oim_get_mirror_list() {

    LOG_INFO("Attempting to retrieve Mirror list");

    if (cached_mirror_list) {
        LOG_INFO("Returning cached Mirror list");
        return json_object_get(cached_mirror_list);
    }

    json_object *cached_list = oim_cache_get_mirror_list();
    if (cached_list) {
        LOG_INFO("Retrieved Mirror list from cache");
        cached_mirror_list = cached_list;
        return json_object_get(cached_mirror_list);
    }

    LOG_INFO("No cached list found. Attempting to rescan Mirror directory");
    int scan_result = oim_rescan_mirror_directory();
    if (scan_result != 0) {
        LOG_ERROR("Mirror directory rescan failed");
        return NULL;
    }

    if (cached_mirror_list == NULL) {
        LOG_ERROR("Mirror list is still NULL after rescan");
        return NULL;
    }

    LOG_INFO("Successfully retrieved Mirror list");
    return json_object_get(cached_mirror_list);
}

void oim_free_mirror_entries() {

}