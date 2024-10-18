#ifndef SERVER_H
#define SERVER_H

#define BUFFER_SIZE 1024
#define MAX_HEADERS 64

enum ERR_TYPES {
    NFOUND,
    BADREQ,
    INTERR,
};

typedef struct {
    char *name;
    char *value;
} Header;

typedef struct {
    char *method;
    char *path;
    char *version;
    Header *headers;
    int headers_len;
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


int parse_http_req(int client_fd, const char *buffer, HttpRequest *http_req);
int serve_file(int client_fd, const char *path);
void handle_client(int client_fd);
void handle_sigint(int sig);

void send_json_response(int client_fd, char *json);

#endif
