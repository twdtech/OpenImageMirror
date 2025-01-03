#ifndef OIM_API_H
#define OIM_API_H

#include <microhttpd.h>
#include <stdbool.h>

typedef enum MHD_Result (*OIMAPIRequestHandler)(
    void *cls, 
    struct MHD_Connection *connection, 
    const char *url, 
    const char *method, 
    const char *version, 
    const char *upload_data, 
    size_t *upload_data_size, 
    void **ptr
);

typedef struct {
    int port;
} OIMAPIServerConfig;


enum MHD_Result oim_api_request_handler(
    void *cls,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **ptr
);

void stop_oim_api_server(void);

struct MHD_Daemon *start_oim_api_server(OIMAPIServerConfig *config);

int send_oim_json_response(
    struct MHD_Connection *connection, 
    const char *json_str, 
    int status_code
);

#endif