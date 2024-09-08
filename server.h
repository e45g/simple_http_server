#ifndef SERVER_H
#define SERVER_H

#define PORT 3000
#define BUFFER_SIZE 1024

#define FRONTEND "./frontend"
#define BACKEND "./routes"

#define CATCHALL "./catchall"

typedef struct {
    char method[16];
    char path[256];
    char version[16];
} http_request;

typedef struct s_route{
    char method[16];
    char path[265];
    void (*callback)(int client_fd, http_request *req);
    struct s_route *next;
} route;


void parse_http_req(char *buffer, http_request *http_req);
const char *get_mime_type(const char *path);
void handle_file_request(int client_fd, const char *file, int flags);
void handle_client(int client_fd);
void handle_sigint(int sig);

#endif
