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
};

void handle_error(char *msg, int sckt){
    perror(msg);
    if(sckt > 0) close(sckt);
    exit(EXIT_FAILURE);
}

void send_error_response(int client_fd, int status_code, char *message){
    char body[512];
    snprintf(body, sizeof(body), "<html><body><h1>%d %s</h1></body></html>", status_code, message);

    char response[1024];
    snprintf(response, sizeof(response), "HTTP/1.1 %d %s\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n%s",
             status_code, message, strlen(body), body);
    send(client_fd, response, strlen(response), 0);
}

int validate_request(const HttpRequest *req){
    return strlen(req->path) > 0 && strlen(req->method) > 0 && strlen(req->version) > 0;
}

void parse_http_req(char *buffer, HttpRequest *http_req){
   sscanf(buffer, "%s %s %s", http_req->method, http_req->path, http_req->version);
}

const char *get_mime_type(const char *path) {
    const char *ext = strchr(path, '.');
    if(!ext) return "application/octet-stream";

    for(int i = 0; i < sizeof(mime_types)/sizeof(mime_types[0]); i++){
        if(strcmp(mime_types[i].extension, ext) == 0){
            return mime_types[i].extension;
        }
    }

    return "application/octet-stream";
}

int serve_file(int client_fd, const char *path){
    char p[512];
    snprintf(p, 512, "%s/%s", ROUTES, path);
    LOG("Path: %s", p);
    int filefd = open(p, O_RDONLY);
    if (filefd == -1) {
        snprintf(p, 512, "%s/%s", CATCHALL, path);
        LOG("Path: %s", p);
        filefd = open(p, O_RDONLY);
        if(filefd == -1){
            send_error_response(client_fd, 404, NOT_FOUND_MSG);
            return -1;
        }
    }

    struct stat st;
    if (fstat(filefd, &st) == -1) {
        close(filefd);
        send_error_response(client_fd, 500, "Internal Server Error");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    const char *mime_type = get_mime_type(path);
    snprintf(buffer, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n" \
                                  "Content-Type: %s\r\n" \
                                  "Content-Length: %ld\r\n\r\n",
                                  mime_type, st.st_size);

    send(client_fd, buffer, strlen(buffer), 0);

    char file_buffer[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = read(filefd, file_buffer, sizeof(file_buffer))) > 0) {
        send(client_fd, file_buffer, bytes, 0);
    }

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
    LOG("\n%s", buffer);

    HttpRequest req = {0};
    parse_http_req(buffer, &req);

    if(!validate_request(&req)){
        send_error_response(client_fd, 400, "Bad HTTP request.");
        return;
    }

    for(Route *r = server.route; r; r = r->next){
        if(strcmp(req.method, r->method) == 0 && match_route(req.path, r->path)){
            r->callback(client_fd, &req);
            return;
        }
    }

    if(strcmp(req.method, "GET") == 0){
        serve_file(client_fd, req.path+1);
    }

    send_error_response(client_fd, 404, NOT_FOUND_MSG);
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
    signal(SIGINT, handle_sigint);

    int sckt = socket(AF_INET, SOCK_STREAM, 0);
    if (sckt < 0) handle_error("Socket creation falied.", -1);
    server.sckt = sckt;
    server.route = NULL;

    const struct sockaddr_in addr = {
        AF_INET,
        htons(PORT),
        0,
    };

    int opt = 1;
    if (setsockopt(sckt, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        handle_error("setsockopt failed.", server.sckt);
    }

    if (bind(sckt, (struct sockaddr*)&addr, sizeof(addr)) != 0){
        handle_error("Bind failed.", server.sckt);
    };

    if(listen(sckt, 10) != 0){
        handle_error("Listen failed.", server.sckt);
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
