#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>
#include <stdio.h>

#define LOG_ERROR(msg, ...) log_message(LOG_ERROR, msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...)  log_message(LOG_WARN, msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...)  log_message(LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) log_message(LOG_DEBUG, msg, ##__VA_ARGS__)

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

int init_logging(const char *log_file_path, LogLevel level, bool enable_logging);
void log_message(LogLevel level, const char *format, ...);
void close_logging(void);

#endif