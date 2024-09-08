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

#include "routes.h"

#define PORT 8080
#define BUFFER_SIZE 1024

#define RESPONSE_NOT_FOUND "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"
#define RESPONSE_OK "HTTP/1.1 200 OK\r\n"

#define FRONTEND "./frontend"
#define BACKEND "./routes"

typedef struct {
    char method[16];
    char path[256];
    char version[16];
} http_request;

void parse_http_req(char *buffer, http_request *http_req){
   sscanf(buffer, "%s %s %s", http_req->method, http_req->path, http_req->version);
}

const char *get_mime_type(const char *path) {
    const char *ext = strchr(path, '.');
    if(!ext) return "application/octet-stream";

    if (strcmp(ext, ".aac") == 0) return "audio/aac";
    if (strcmp(ext, ".bpm") == 0) return "image/bmp";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".csv") == 0) return "text/csv";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    return "application/octet-stream";
}

void handle_file_request(int client_fd, const char *file){
    int opened_fd = open(file, O_RDONLY);
    if (opened_fd == -1){
        printf("File not found.\n");
        send(client_fd, RESPONSE_NOT_FOUND, sizeof(RESPONSE_NOT_FOUND) - 1, 0);
        return;
    }

    struct stat file_stat;
    fstat(opened_fd, &file_stat);
    char response_header[BUFFER_SIZE];

    snprintf(response_header, sizeof(response_header),
             "%sContent-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             RESPONSE_OK, get_mime_type(file), file_stat.st_size);

    ssize_t offset = 0;
    send(client_fd, response_header, strlen(response_header), 0);
    sendfile(client_fd, opened_fd, &offset, file_stat.st_size);
    close(opened_fd);
}

void handle_client(int client_fd){
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
    if (bytes_recieved < 0){
        printf("recv failed.\n");
        close(client_fd);
        return;
    }
    buffer[bytes_recieved] = '\0';
    printf("%s\n", buffer);

    http_request req;
    parse_http_req(buffer, &req);

    if(strcmp(req.method, "GET") == 0){
        if (strlen(req.path) == 0 || strcmp(req.path, "/") == 0) {
            strcpy(req.path, "/index.html");
        }
        char *file = req.path + 1;
        handle_file_request(client_fd, file);
    } else {
        send(client_fd, RESPONSE_NOT_FOUND, strlen(RESPONSE_NOT_FOUND), 0);
    }
    close(client_fd);
}

void handle_sigint(int sig) {
    printf("\nShutting down server...\n");
    exit(0);
}

int main(){
    signal(SIGINT, handle_sigint);
	setbuf(stdout, NULL);
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0){
        printf("Socket failed.");
        exit(EXIT_FAILURE);
    }

    const struct sockaddr_in addr = {
        AF_INET,
        htons(PORT),
        0,
    };

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) != 0){
        perror("Bind failed.");
        close(s);
        exit(EXIT_FAILURE);
    };

    if(listen(s, 10) != 0){
        perror("Listen failed.");
        close(s);
        exit(EXIT_FAILURE);
    };

    printf("Server listening on port %d\n", PORT);

    while(1){
        int client_fd = accept(s, NULL, NULL);
        if (client_fd < 0) {
            perror("Accept failed.");
            close(s);
            continue;
        }
        handle_client(client_fd);
    }
    close(s);
    return 0;
}
