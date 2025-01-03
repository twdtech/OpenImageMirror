#ifndef OIM_MIRROR_MANAGER_H
#define OIM_MIRROR_MANAGER_H

#include <stdbool.h>
#include <json-c/json.h>
#include "config.h"

typedef struct {
    char *filename;         
    char *path;             
    char *category;         
    char *subcategory;      
    long long file_size;    
    time_t modified_time;   
} OIMMirrorEntry;

typedef struct {
    char *base_directory;   
    bool recursive_scan;    
    int scan_interval;      
} OIMMirrorManagerConfig;

int oim_init_mirror_manager(OIMConfig *config);
void oim_cleanup_mirror_manager();

json_object* oim_get_mirror_list();
int oim_rescan_mirror_directory();

void oim_free_mirror_entries();
char* oim_generate_category_from_path(const char *full_path, const char *base_dir);

int oim_scan_directory(
    const char *directory, 
    const char *base_directory, 
    json_object *mirror_list
);

#endif 