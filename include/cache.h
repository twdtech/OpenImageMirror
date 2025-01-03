#ifndef OIM_CACHE_H
#define OIM_CACHE_H

#include <sqlite3.h>
#include <stdbool.h>
#include <json-c/json.h>
#include <time.h>

typedef struct {
    char *db_path;          
    int cache_expiry_time;  
} OIMMirrorCacheConfig;

typedef struct {
    sqlite3 *db;            
    bool is_initialized;    
} OIMMirrorCacheInitResult;

int oim_init_cache(OIMMirrorCacheConfig *config);
void oim_close_cache();

int oim_cache_store_mirror_list(json_object *mirror_list);
json_object* oim_cache_get_mirror_list();
bool oim_is_cache_valid();

int oim_create_cache_schema(sqlite3 *db);
int64_t oim_get_current_timestamp();
void oim_cleanup_old_cache_entries();

#endif 