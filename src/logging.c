#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "logging.h"

static FILE *log_file = NULL;
static LogLevel current_log_level = LOG_INFO;
static bool logging_enabled = false;

static const char* get_log_level_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKNOWN";
    }
}

int init_logging(const char *log_file_path, LogLevel level, bool enable_logging) {

    close_logging();

    if (!enable_logging) {
        logging_enabled = false;
        return 0;
    }

    if (log_file_path == NULL || strlen(log_file_path) == 0) {
        fprintf(stderr, "Invalid log file path\n");
        return -1;
    }

    log_file = fopen(log_file_path, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Failed to open log file: %s\n", log_file_path);
        return -1;
    }

    logging_enabled = true;
    current_log_level = level;

    return 0;
}

void log_message(LogLevel level, const char *format, ...) {
    if (!logging_enabled || level < current_log_level) {
        return;
    }

    time_t now;
    struct tm *timestamp;
    char time_buffer[64];

    time(&now);
    timestamp = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timestamp);

    va_list args;
    va_start(args, format);

    if (level >= LOG_WARN) {
        FILE *output_stream = (level == LOG_ERROR) ? stderr : stdout;
        fprintf(output_stream, "[%s] %s: ", time_buffer, get_log_level_string(level));
        vfprintf(output_stream, format, args);
        fprintf(output_stream, "\n");
    }

    if (log_file) {
        fprintf(log_file, "[%s] %s: ", time_buffer, get_log_level_string(level));
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }

    va_end(args);
}

void close_logging(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    logging_enabled = false;
}