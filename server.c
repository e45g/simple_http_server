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

route *routes = NULL;

void handle_not_found(int client_fd){
        const char *response_body = "<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>";
        char response_header[BUFFER_SIZE];

        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/html\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\n",
                 strlen(response_body));

        send(client_fd, response_header, strlen(response_header), 0);
        send(client_fd, response_body, strlen(response_body), 0);
        close(client_fd);

}

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

char *remove_ext(const char *file) {
    char *f = malloc(sizeof(file));
    strcpy(f, file);
    *(strchr(f, '.')) = '\0';
    return f;
}

void handle_file_request(int client_fd, const char *file){
    char path[256];
    char *file_without_ext = remove_ext(file);

    snprintf(path, sizeof(path), "%s/%s/%s", FRONTEND, file_without_ext, file);
    free(file_without_ext);

    printf("%s\n", path);
    int opened_fd = open(path, O_RDONLY);
    if (opened_fd == -1){
        printf("File not found.\n");
        handle_not_found(client_fd);
        return;
    }

    struct stat file_stat;
    fstat(opened_fd, &file_stat);
    char response_header[BUFFER_SIZE];

    snprintf(response_header, sizeof(response_header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             get_mime_type(file), file_stat.st_size);

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

    route *current_route = routes;
    while (current_route != NULL) {
        if (strcmp(req.method, current_route->method) == 0 && strcmp(req.path, current_route->path) == 0) {
            current_route->callback(client_fd, &req);
            close(client_fd);
            return;
        }
        current_route = current_route->next;
    }
    handle_not_found(client_fd);
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

    load_routes();
    printf("Routes loaded.\n\n");
    print_routes();

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
