#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <time.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_CRITICAL
} LogLevel;

int create_directory(const char *path);
bool file_exists(const char *path);
bool is_directory(const char *path);
long get_file_size(const char *path);

char* str_duplicate(const char *src);
char* str_trim(char *str);
char* str_to_lowercase(char *str);
bool str_ends_with(const char *str, const char *suffix);
char* generate_uuid();

char* path_join(const char *path1, const char *path2);
char* get_absolute_path(const char *path);
char* get_file_extension(const char *filename);
char* get_filename_without_extension(const char *filename);

void log_message(LogLevel level, const char *format, ...);
void set_log_file(const char *log_path);
void set_log_level(LogLevel level);

char* get_current_timestamp_str();
time_t parse_timestamp(const char *timestamp_str);
int get_days_between_dates(time_t date1, time_t date2);

char* generate_random_string(int length);
char* hash_string(const char *input);

long get_available_disk_space(const char *path);
int get_cpu_count();
long get_total_memory();

#endif 