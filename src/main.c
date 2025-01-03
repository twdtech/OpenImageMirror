#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "config.h"
#include "api.h"
#include "imgMgr.h"
#include "cache.h"
#include "logging.h"

OIMConfig *global_config = NULL;
struct MHD_Daemon *global_daemon = NULL;

void oim_cleanup_resources() {
    LOG_INFO("Performing cleanup of resources");

    if (global_daemon) {
        stop_oim_api_server();
        global_daemon = NULL;
    }

    close_logging();

    if (global_config) {
        oim_free_config(global_config);
        global_config = NULL;
    }

    oim_close_cache();
}

void oim_signal_handler(int signum) {
    LOG_WARN("Received signal %d. Initiating shutdown...", signum);

    oim_cleanup_resources();
    exit(0);
}

int main(void) {

    signal(SIGINT, oim_signal_handler);
    signal(SIGTERM, oim_signal_handler);

    if (atexit(oim_cleanup_resources) != 0) {
        fprintf(stderr, "Failed to register exit handler\n");
        return 1;
    }

    global_config = oim_load_config("config/config.json");
    if (global_config == NULL) {
        fprintf(stderr, "Failed to load configuration\n");
        LOG_ERROR("Configuration loading failed");
        return 1;
    }

    if (oim_init_mirror_manager(global_config) != 0) {
        LOG_ERROR("Failed to initialize Mirror manager");
        return 1;
    }

    if (init_logging(
        global_config->log_file_path, 
        global_config->debug_mode ? LOG_DEBUG : LOG_INFO, 
        global_config->enable_logging
    ) != 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return 1;
    }

    LOG_INFO("Configuration loaded successfully");

    OIMMirrorCacheConfig cache_config = {
        .db_path = global_config->cache_db_path,
        .cache_expiry_time = global_config->cache_expiry_time
    };

    if (oim_init_cache(&cache_config) != 0) {
        LOG_ERROR("Failed to initialize cache");
        return 1;
    }

    LOG_INFO("Cache initialized successfully");

    OIMAPIServerConfig api_config = {
        .port = global_config->api_port
    };

    global_daemon = start_oim_api_server(&api_config);
    if (global_daemon == NULL) {
        LOG_ERROR("Failed to start API server");
        return 1;
    }

    LOG_INFO("API server started on port %d", global_config->api_port);

    LOG_INFO("Server running. Waiting for requests...");
    while (1) {
        sleep(1);
    }

    return 0;
}