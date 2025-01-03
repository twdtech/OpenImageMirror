#ifndef OIM_CONFIG_H
#define OIM_CONFIG_H

#include <stdbool.h>
#include <json-c/json.h>

typedef struct {

    int api_port;           
    char *mirror_directory;    

    char *cache_db_path;    
    int cache_expiry_time;  

    int scan_interval;      
    bool recursive_scan;    

    bool enable_logging;    
    char *log_file_path;    

    bool debug_mode;        
} OIMConfig;

OIMConfig* oim_load_config(const char *config_path);
void oim_free_config(OIMConfig *config);

char* oim_get_string_value(json_object *json, const char *key, const char *default_value);
int oim_get_int_value(json_object *json, const char *key, int default_value);
bool oim_get_bool_value(json_object *json, const char *key, bool default_value);

#endif 