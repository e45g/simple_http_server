#ifndef SERVER_H
#define SERVER_H

#define BUFFER_SIZE 1024
#define NOT_FOUND_MSG "File Not Found."

typedef struct {
    char method[16];
    char path[256];
    char version[16];
} HttpRequest;

typedef struct s_route {
    char method[16];
    char path[265];
    void (*callback)(int client_fd, HttpRequest *req);
    struct s_route *next;
} Route;

typedef struct {
    char extension[16];
    char mime_type[64];
} MimeEntry;

typedef struct {
    int sckt;
    Route *route;
} Server;


void parse_http_req(char *buffer, HttpRequest *http_req);
int serve_file(int client_fd, const char *path);
void handle_client(int client_fd);
void handle_sigint(int sig);

void send_json_response(int client_fd, char *json);

#endif
