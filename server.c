#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "server.h"
#include "routes.h"
#include "utils.h"

Server server;
MimeEntry mime_types[] = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".gif", "image/gif"},
        {".txt", "text/plain"},
        {".json", "application/json"}
};

void handle_critical_error(char *msg, int sckt){
    perror(msg);
    if(sckt > 0) close(sckt);
    exit(EXIT_FAILURE);
}

void send_error_response(int client_fd, int error_type){
    int status_code;
    char msg[128];

    switch (error_type) {
        case NFOUND:
            status_code = 404;
            strcpy(msg, "File Not Found.");
            LOG("%d %s", status_code, msg);
            break;

        case BADREQ:
            status_code = 400;
            strcpy(msg, "Bad Request.");
            break;

        case INTERR:
            status_code = 500;
            strcpy(msg, "Internal Server Error.");
            break;
    }

    char body[512];
    snprintf(body, sizeof(body), "<html><body><h1>%d %s</h1></body></html>", status_code, msg);

    char response[1024];
    snprintf(response, sizeof(response), "HTTP/1.1 %d %s\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n%s",
             status_code, msg, strlen(body), body);
    send(client_fd, response, strlen(response), 0);
}

void send_json_response(int client_fd, char *json){
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", strlen(json), json);
    send(client_fd, response, strlen(response), 0);
}

int validate_request(const HttpRequest *req){
    return strlen(req->path) > 0 && strlen(req->method) > 0 && strlen(req->version) > 0;
}

int parse_http_req(int client_fd, const char *buffer, HttpRequest *http_req){
    char *buf = strdup(buffer);
    if(!buf) {
        perror("Memory allocation failed. (parse_http_req)");
        return -2;
    }

    char *line_end = strstr(buf, "\r\n");
    if(!line_end) {
        free(buf);
        return -1;
    }

    *line_end = '\0';

    char *method_end = strchr(buf, ' ');
    if(!method_end) {
        free(buf);
        return -1;
    }

    *method_end = '\0';
    http_req->method = strdup(buf);
    if(!http_req->method){
        perror("Mem allocation failed. (method)");
        free(buf);
        return -2;
    }

    char *path_start = method_end + 1;
    char *path_end = strchr(path_start, ' ');
    if(!path_end){
        free(buf);
        return -1;
    }

    *path_end = '\0';
    http_req->path = strdup(path_start);
    if(!http_req->path){
        perror("Mem allocation failed. (path)");
        free(buf);
        return -2;
    }

    char *version_start = path_end + 1;
    http_req->version = strdup(version_start);
    if (!http_req->version) {
        perror("Memory allocation failed. (version)");
        free(buf);
        return -2;
    }

    http_req->headers_len = 0;
    http_req->headers = malloc(sizeof(Header) * MAX_HEADERS);

    if(!http_req->headers){
        perror("Memory allocation failed. (headers)");
        free(buf);
        return -2;
    }

    char *header_line = line_end + 2;
    while(*header_line && http_req->headers_len < MAX_HEADERS){
        char *header_end = strstr(header_line, "\r\n");
        if(!header_end) break;

        *header_end = '\0';

        char *colon = strchr(header_line, ':');
        if(!colon){
            header_line += 2;
            continue;
        }

        *colon = '\0';
        char *header_name = header_line;
        char *header_value = colon + 1;

        while(*header_value == ' ') header_value++;

        http_req->headers[http_req->headers_len].name = strdup(header_name);
        http_req->headers[http_req->headers_len].value = strdup(header_value);

        if (!http_req->headers[http_req->headers_len].name || !http_req->headers[http_req->headers_len].value) {
            perror("Memory allocation failed. (header fields)");
            header_end += 2;
            continue;
        }

        http_req->headers_len++;
        header_line = header_end + 2;
    }
    free(buf);

    return 0;
}

void free_http_req(HttpRequest *req){
    free(req->method);
    free(req->path);
    free(req->version);

    for (int i = 0; i < req->headers_len; i++) {
        free(req->headers[i].name);
        free(req->headers[i].value);
    }
    free(req->headers);
}

const char *get_mime_type(const char *path) {
    const char *ext = strchr(path, '.');
    if(!ext) return "application/octet-stream";

    for(int i = 0; i < sizeof(mime_types)/sizeof(mime_types[0]); i++){
        if(strcmp(mime_types[i].extension, ext) == 0){
            return mime_types[i].mime_type;
        }
    }

    return "application/octet-stream";
}

int serve_file(int client_fd, const char *path){
    char p[512];

    snprintf(p, 512, "%s/%s", get_routes_dir(), path);
    LOG("Path: %s", p);
    int filefd = open(p, O_RDONLY);

    if (filefd == -1) {
        snprintf(p, 512, "%s/%s", get_public_dir(), path);
        LOG("Path: %s", p);
        filefd = open(p, O_RDONLY);

        if(filefd == -1){
            LOG("Not found");
            send_error_response(client_fd, NFOUND);
            return -1;
        }
    }

    struct stat st;
    if (fstat(filefd, &st) == -1) {
        close(filefd);
        send_error_response(client_fd, INTERR);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    const char *mime_type = get_mime_type(path);
    snprintf(buffer, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n" \
                                  "Content-Type: %s\r\n" \
                                  "Content-Length: %ld\r\n\r\n",
                                  mime_type, st.st_size);

    send(client_fd, buffer, strlen(buffer), 0);

    ssize_t offset = 0;
    sendfile(client_fd, filefd, &offset, st.st_size);

    close(filefd);
    return 0;
}

void handle_client(int client_fd){
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE-1, 0);

    if (bytes_recieved < 0){
        LOG("RECV failed.");
        return;
    }
    buffer[bytes_recieved] = '\0';

    HttpRequest req = {0};
    int status = parse_http_req(client_fd, buffer, &req);

    if (status < 0){
        if(status == -1){
            send_error_response(client_fd, BADREQ);
        } else if (status == -2) {
            send_error_response(client_fd, INTERR);
        }
        free_http_req(&req);
        return;
    }

    if(!validate_request(&req)){
        send_error_response(client_fd, BADREQ);
        free_http_req(&req);
        return;
    }

    for(Route *r = server.route; r; r = r->next){
        if(strcmp(req.method, r->method) == 0 && match_route(req.path, r->path)){
            r->callback(client_fd, &req);
            free_http_req(&req);
            return;
        }
    }

    if(strcmp(req.method, "GET") == 0){
        serve_file(client_fd, req.path+1);
    }

    send_error_response(client_fd, NFOUND);
    free_http_req(&req);
}

void handle_sigint(int sig) {
    LOG("Shutting down server...");
    if(server.sckt) {
        close(server.sckt);
    }
    free_routes(server.route);
    exit(0);
}

int main(){
    load_env(".env");
    signal(SIGINT, handle_sigint);

    const int PORT = get_port();

    int sckt = socket(AF_INET, SOCK_STREAM, 0);
    if (sckt < 0) handle_critical_error("Socket creation falied.", -1);
    server.sckt = sckt;
    server.route = NULL;

    const struct sockaddr_in addr = {
        AF_INET,
        htons(PORT),
        0,
    };

    int opt = 1;
    if (setsockopt(sckt, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        handle_critical_error("setsockopt failed.", server.sckt);
    }

    if (bind(sckt, (struct sockaddr*)&addr, sizeof(addr)) != 0){
        handle_critical_error("Bind failed.", server.sckt);
    };

    if(listen(sckt, 10) != 0){
        handle_critical_error("Listen failed.", server.sckt);
    };

    printf("Server listening on port %d\n", PORT);

    load_routes();
    printf("Routes loaded.\n\n");
    print_routes();

    while(1){
        int client_fd = accept(server.sckt, NULL, NULL);
        if (client_fd < 0) {
            perror("Accept failed.");
            continue;
        }
        handle_client(client_fd);
        close(client_fd);
    }

    close(sckt);
    return 0;
}
