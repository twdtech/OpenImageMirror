#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <json-c/json.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "cache.h"
#include "config.h"
#include "logging.h"

static sqlite3 *oim_cache_db = NULL;
static OIMMirrorCacheConfig *current_config = NULL;

int oim_ensure_directory_exists(const char *path) {
    char *dir_path = strdup(path);
    char *last_slash = strrchr(dir_path, '/');
    
    if (last_slash) {
        *last_slash = '\0';
        
        for (char *p = dir_path; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                mkdir(dir_path, 0755);
                *p = '/';
            }
        }
        
        mkdir(dir_path, 0755);
    }
    
    free(dir_path);
    return 0;
}

int oim_init_cache(OIMMirrorCacheConfig *config) {
    if (config == NULL) {
        LOG_ERROR("Cache configuration is NULL");
        return -1;
    }

    if (config->db_path == NULL || strlen(config->db_path) == 0) {
        LOG_ERROR("Invalid cache database path");
        return -1;
    }

    LOG_INFO("Attempting to create cache directory: %s", config->db_path);

    char *dir_path = strdup(config->db_path);
    if (dir_path == NULL) {
        LOG_ERROR("Memory allocation failed for directory path");
        return -1;
    }

    char *last_slash = strrchr(dir_path, '/');
    if (last_slash == NULL) {
        LOG_ERROR("Invalid path: no directory separator found");
        free(dir_path);
        return -1;
    }

    *last_slash = '\0';

    LOG_INFO("Processing directory path: %s", dir_path);

    char current_path[1024] = {0};
    char *token = strtok(dir_path, "/");

    while (token != NULL) {
        if (current_path[0] == '\0') {
            snprintf(current_path, sizeof(current_path), "/%s", token);
        } else {
            strncat(current_path, "/", sizeof(current_path) - strlen(current_path) - 1);
            strncat(current_path, token, sizeof(current_path) - strlen(current_path) - 1);
        }

        LOG_INFO("Attempting to create directory: %s", current_path);
        
        if (mkdir(current_path, 0755) != 0) {
            if (errno != EEXIST) {
                LOG_ERROR("Failed to create directory: %s (Error: %s)", 
                          current_path, strerror(errno));
                free(dir_path);
                return -1;
            } else {
                LOG_INFO("Directory already exists: %s", current_path);
            }
        } else {
            LOG_INFO("Successfully created directory: %s", current_path);
        }

        token = strtok(NULL, "/");
    }

    free(dir_path);

    current_config = malloc(sizeof(OIMMirrorCacheConfig));
    if (current_config == NULL) {
        LOG_ERROR("Failed to allocate memory for cache configuration");
        return -1;
    }
    memcpy(current_config, config, sizeof(OIMMirrorCacheConfig));

    int rc = sqlite3_open(config->db_path, &oim_cache_db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Cannot open database: %s", 
                  sqlite3_errmsg(oim_cache_db ? oim_cache_db : NULL));
        
        if (oim_cache_db) {
            sqlite3_close(oim_cache_db);
            oim_cache_db = NULL;
        }
        
        free(current_config);
        current_config = NULL;
        
        return -1;
    }

    rc = oim_create_cache_schema(oim_cache_db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to create cache schema");
        oim_close_cache();
        return -1;
    }

    LOG_INFO("Cache initialized successfully at %s", config->db_path);
    return 0;
}


int oim_create_cache_schema(sqlite3 *db) {
    const char *create_table_sql = 
        "CREATE TABLE IF NOT EXISTS mirror_cache ("
        "   id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   mirror_list TEXT,"
        "   timestamp INTEGER"
        ");";

    char *err_msg = 0;
    int rc = sqlite3_exec(db, create_table_sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    return SQLITE_OK;
}

int oim_cache_store_mirror_list(json_object *mirror_list) {
    if (oim_cache_db == NULL || mirror_list == NULL) {
        return -1;
    }

    const char *mirror_list_str = json_object_to_json_string(mirror_list);
    int64_t current_time = oim_get_current_timestamp();

    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO mirror_cache (mirror_list, timestamp) VALUES (?, ?)";
    
    int rc = sqlite3_prepare_v2(oim_cache_db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement\n");
        return -1;
    }

    sqlite3_bind_text(stmt, 1, mirror_list_str, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, current_time);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    oim_cleanup_old_cache_entries();

    return (rc == SQLITE_DONE) ? 0 : -1;
}

json_object* oim_cache_get_mirror_list() {
    if (oim_cache_db == NULL || !oim_is_cache_valid()) {
        return NULL;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT mirror_list FROM mirror_cache ORDER BY timestamp DESC LIMIT 1";
    
    int rc = sqlite3_prepare_v2(oim_cache_db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement\n");
        return NULL;
    }

    rc = sqlite3_step(stmt);
    json_object *mirror_list = NULL;

    if (rc == SQLITE_ROW) {
        const unsigned char *json_str = sqlite3_column_text(stmt, 0);
        if (json_str ) {
            mirror_list = json_tokener_parse((const char *)json_str);
        }
    }

    sqlite3_finalize(stmt);
    return mirror_list;
}

bool oim_is_cache_valid() {
    if (oim_cache_db == NULL || current_config == NULL) {
        return false;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT timestamp FROM mirror_cache ORDER BY timestamp DESC LIMIT 1";
    
    int rc = sqlite3_prepare_v2(oim_cache_db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return false;
    }

    rc = sqlite3_step(stmt);
    bool is_valid = false;

    if (rc == SQLITE_ROW) {
        int64_t last_timestamp = sqlite3_column_int64(stmt, 0);
        int64_t current_time = oim_get_current_timestamp();
        
        is_valid = (current_time - last_timestamp) <= current_config->cache_expiry_time;
    }

    sqlite3_finalize(stmt);
    return is_valid;
}

int64_t oim_get_current_timestamp() {
    return (int64_t)time(NULL);
}

void oim_cleanup_old_cache_entries() {
    if (oim_cache_db == NULL || current_config == NULL) {
        return;
    }

    int64_t current_time = oim_get_current_timestamp();
    sqlite3_stmt *stmt;

    const char *sql = "DELETE FROM mirror_cache WHERE timestamp < ?";
    
    int rc = sqlite3_prepare_v2(oim_cache_db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare cleanup statement\n");
        return;
    }

    sqlite3_bind_int64(stmt, 1, current_time - current_config->cache_expiry_time);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void oim_close_cache() {
    if (oim_cache_db) {
        sqlite3_close(oim_cache_db);
        oim_cache_db = NULL;
    }

    if (current_config) {
        free(current_config);
        current_config = NULL;
    }
}