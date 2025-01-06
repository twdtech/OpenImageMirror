#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include <json-c/json.h>
#include <sqlite3.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define _XOPEN_SOURCE 500
#include <libgen.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "api.h"
#include "config.h"
#include "imgMgr.h"
#include "logging.h"

static struct MHD_Daemon *oim_api_server = NULL;
static OIMConfig *global_config = NULL;

static int is_safe_path(const char *path) {
    if (strstr(path, "..") != NULL) {
        return 0;
    }
    
    if (path[0] == '/') {
        return 0;
    }
    
    return 1;
}

static char* oim_safe_basename(const char* path) {
    char* base = strrchr(path, '/');
    return base ? base + 1 : (char*)path;
}

int send_oim_json_response(
    struct MHD_Connection *connection, 
    const char *json_str, 
    int status_code
) {
    struct MHD_Response *response;
    int ret;

    response = MHD_create_response_from_buffer(
        strlen(json_str),
        (void *)json_str,
        MHD_RESPMEM_MUST_COPY
    );

    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET");

    ret = MHD_queue_response(connection, status_code, response);
    
    MHD_destroy_response(response);

    return ret;
}

enum MHD_Result oim_api_request_handler(
    void *cls __attribute__((unused)), 
    struct MHD_Connection *connection, 
    const char *url, 
    const char *method __attribute__((unused)), 
    const char *version __attribute__((unused)), 
    const char *upload_data __attribute__((unused)), 
    size_t *upload_data_size __attribute__((unused)), 
    void **ptr __attribute__((unused))
) {
    const union MHD_ConnectionInfo *conn_info = 
    MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    
    char client_ip[INET6_ADDRSTRLEN] = "Unknown";
    if (conn_info && conn_info->client_addr) {
        inet_ntop(conn_info->client_addr->sa_family, 
                  conn_info->client_addr, 
                  client_ip, 
                  sizeof(client_ip));
    }

    if (strcmp(url, "/api/mirror") == 0) {
        LOG_INFO("Mirror list request from IP: %s", client_ip);

        json_object *mirror_list = oim_get_mirror_list();
        
        if (mirror_list == NULL) {
            LOG_ERROR("Failed to retrieve mirror list for IP: %s", client_ip);
            return send_oim_json_response(connection, 
                "{\"error\": \"Failed to retrieve mirror list\"}", 
                MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        const char *json_str = json_object_to_json_string(mirror_list);
        
        int ret = send_oim_json_response(connection, json_str, MHD_HTTP_OK);
        
        json_object_put(mirror_list);
        
        return ret;
    }

    if (strncmp(url, "/download/", 10) == 0) {
        const char *file_path = url + 10;

        LOG_INFO("Download request from IP: %s for file: %s", client_ip, file_path);

        if (global_config == NULL) {
            LOG_ERROR("Server configuration not loaded for download request from IP: %s", client_ip);
            return send_oim_json_response(connection, 
                "{\"error\": \"Server configuration not loaded\"}", 
                MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        if (!is_safe_path(file_path)) {
            LOG_WARN("Unsafe download path attempted from IP: %s, Path: %s", client_ip, file_path);
            return send_oim_json_response(connection, 
                "{\"error\": \"Invalid file path\"}", 
                MHD_HTTP_BAD_REQUEST);
        }
        
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", 
                 global_config->mirror_directory,  
                 file_path);

        int fd = open(full_path, O_RDONLY);
        if (fd == -1) {
            LOG_ERROR("Failed to open file for download from IP: %s, File: %s, Error: %s", 
                      client_ip, full_path, strerror(errno));
            
            return send_oim_json_response(connection, 
                "{\"error\": \"File not found\"}", 
                MHD_HTTP_NOT_FOUND);
        }

        off_t file_size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        struct MHD_Response *response;
        response = MHD_create_response_from_fd(file_size, fd);
        
        if (response == NULL) {
            close(fd);
            return send_oim_json_response(connection, 
                "{\"error\": \"Failed to create response\"}", 
                MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        char content_disposition[256];
        snprintf(content_disposition, sizeof(content_disposition), 
                 "attachment; filename=\"%s\"", oim_safe_basename(file_path));
        
        MHD_add_response_header(response, "Content-Type", "application/octet-stream");
        MHD_add_response_header(response, "Content-Disposition", content_disposition);
        
        char content_length[32];
        snprintf(content_length, sizeof(content_length), "%ld", (long)file_size);
        MHD_add_response_header(response, "Content-Length", content_length);

        enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        
        MHD_destroy_response(response);
        
        return ret;
    }

    return send_oim_json_response(connection, 
        "{\"error\": \"Not Found\"}", 
        MHD_HTTP_NOT_FOUND);
}

struct MHD_Daemon *start_oim_api_server(OIMAPIServerConfig *config, OIMConfig *oim_config) {
    global_config = oim_config;

    if (oim_api_server != NULL) {
        fprintf(stderr, "API server already running\n");
        return oim_api_server;
    }
    
    oim_api_server = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD,
        config->port,
        NULL, 
        NULL,
        (MHD_AccessHandlerCallback)oim_api_request_handler, 
        NULL, 
        MHD_OPTION_END
    );
    
    if (oim_api_server == NULL) {
        fprintf(stderr, "Failed to start API server\n");
        return NULL;
    }
    
    printf("API server started on port %d\n", config->port);
    return oim_api_server;
}

void stop_oim_api_server(void) {
    if (oim_api_server != NULL) {
        MHD_stop_daemon(oim_api_server);
        oim_api_server = NULL;
        global_config = NULL;
        printf("API server stopped\n");
    }
}