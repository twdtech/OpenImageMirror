#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <json-c/json.h>

#include "config.h"

OIMConfig* oim_load_config(const char *config_path) {

    fprintf(stderr, "Loading configuration from: %s\n", config_path);

    FILE *file = fopen(config_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening config file: %s (Error: %s)\n", 
                config_path, strerror(errno));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "Configuration file is empty or could not determine size\n");
        fclose(file);
        return NULL;
    }

    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation error for config file buffer\n");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';
    fclose(file);

    if ((long)read_size != file_size) {
        fprintf(stderr, "Failed to read entire configuration file\n");
        free(buffer);
        return NULL;
    }

    json_object *json_config = json_tokener_parse(buffer);
    free(buffer);

    if (json_config == NULL) {
        fprintf(stderr, "Error parsing JSON configuration\n");
        return NULL;
    }

    OIMConfig *config = malloc(sizeof(OIMConfig));
    if (config == NULL) {
        fprintf(stderr, "Memory allocation error for config structure\n");
        json_object_put(json_config);
        return NULL;
    }

    config->api_port = oim_get_int_value(json_config, "api_port", 8080);
    fprintf(stderr, "API Port: %d\n", config->api_port);

    config->mirror_directory = oim_get_string_value(
        json_config, 
        "mirror_directory", 
        "/Mirrors"
    );
    fprintf(stderr, "Mirror Directory: %s\n", config->mirror_directory);

    DIR *mirror_dir = opendir(config->mirror_directory);
    if (mirror_dir == NULL) {
        fprintf(stderr, "Warning: Cannot open Mirror directory: %s (Error: %s)\n", 
                config->mirror_directory, strerror(errno));
    } else {
        closedir(mirror_dir);
    }

    config->cache_db_path = oim_get_string_value(
        json_config, 
        "cache_db_path", 
        "/var/cache/open_image_mirror/cache.db"
    );
    fprintf(stderr, "Cache DB Path: %s\n", config->cache_db_path);

    config->cache_expiry_time = oim_get_int_value(
        json_config, 
        "cache_expiry_time", 
        3600  
    );
    fprintf(stderr, "Cache Expiry Time: %d seconds\n", config->cache_expiry_time);

    config->scan_interval = oim_get_int_value(
        json_config, 
        "scan_interval", 
        600  
    );
    fprintf(stderr, "Scan Interval: %d seconds\n", config->scan_interval);

    config->recursive_scan = oim_get_bool_value(
        json_config, 
        "recursive_scan", 
        true
    );
    fprintf(stderr, "Recursive Scan: %s\n", 
            config->recursive_scan ? "Enabled" : "Disabled");

    config->enable_logging = oim_get_bool_value(
        json_config, 
        "enable_logging", 
        true
    );
    fprintf(stderr, "Logging: %s\n", 
            config->enable_logging ? "Enabled" : "Disabled");

    config->log_file_path = oim_get_string_value(
        json_config, 
        "log_file_path", 
        "/var/log/open_image_mirror.log"
    );
    fprintf(stderr, "Log File Path: %s\n", config->log_file_path);

    config->debug_mode = oim_get_bool_value(
        json_config, 
        "debug_mode", 
        false
    );
    fprintf(stderr, "Debug Mode: %s\n", 
            config->debug_mode ? "Enabled" : "Disabled");

    json_object_put(json_config);

    if (config->mirror_directory == NULL) {
        fprintf(stderr, "Error: Mirror directory not set\n");
        free(config);
        return NULL;
    }

    fprintf(stderr, "Configuration loaded successfully\n");
    return config;
}

void oim_free_config(OIMConfig *config) {
    if (config == NULL) return;

    free(config->mirror_directory);
    free(config->cache_db_path);
    free(config->log_file_path);

    free(config);
}

char* oim_get_string_value(
    json_object *json, 
    const char *key, 
    const char *default_value
) {
    json_object *value;
    if (json_object_object_get_ex(json, key, &value)) {
        const char *str_value = json_object_get_string(value);
        return strdup(str_value);
    }
    return strdup(default_value);
}

int oim_get_int_value(
    json_object *json, 
    const char *key, 
    int default_value
) {
    json_object *value;
    if (json_object_object_get_ex(json, key, &value)) {
        return json_object_get_int(value);
    }
    return default_value;
}

bool oim_get_bool_value(
    json_object *json, 
    const char *key, 
    bool default_value
) {
    json_object *value;
    if (json_object_object_get_ex(json, key, &value)) {
        return json_object_get_boolean(value);
    }
    return default_value;
}