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

server_t server = {0, NULL};

void handle_error(char *msg, int sckt){
    perror(msg);
    if(sckt > 0) close(sckt);
    exit(EXIT_FAILURE);
}

int validate_request(const http_request *req){
    return strlen(req->path) > 0 && strlen(req->method) > 0 && strlen(req->version) > 0;
}

void parse_http_req(char *buffer, http_request *http_req){
   sscanf(buffer, "%s %s %s", http_req->method, http_req->path, http_req->version);
}

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

const char *get_mime_type(const char *path) {
    const char *ext = strchr(path, '.');
    if(!ext) return "application/octet-stream";

    if (strcmp(ext, ".aac") == 0) return "audio/aac";
    else if (strcmp(ext, ".abw") == 0) return "application/x-abiword";
    else if (strcmp(ext, ".apng") == 0) return "image/apng";
    else if (strcmp(ext, ".arc") == 0) return "application/x-freearc";
    else if (strcmp(ext, ".avif") == 0) return "image/avif";
    else if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    else if (strcmp(ext, ".azw") == 0) return "application/vnd.amazon.ebook";
    else if (strcmp(ext, ".bin") == 0) return "application/octet-stream";
    else if (strcmp(ext, ".bmp") == 0) return "image/bmp";
    else if (strcmp(ext, ".bz") == 0) return "application/x-bzip";
    else if (strcmp(ext, ".bz2") == 0) return "application/x-bzip2";
    else if (strcmp(ext, ".cda") == 0) return "application/x-cdf";
    else if (strcmp(ext, ".csh") == 0) return "application/x-csh";
    else if (strcmp(ext, ".css") == 0) return "text/css";
    else if (strcmp(ext, ".csv") == 0) return "text/csv";
    else if (strcmp(ext, ".doc") == 0) return "application/msword";
    else if (strcmp(ext, ".docx") == 0) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    else if (strcmp(ext, ".eot") == 0) return "application/vnd.ms-fontobject";
    else if (strcmp(ext, ".epub") == 0) return "application/epub+zip";
    else if (strcmp(ext, ".gz") == 0) return "application/gzip";
    else if (strcmp(ext, ".gif") == 0) return "image/gif";
    else if (strcmp(ext, ".htm") == 0 || strcmp(ext, ".html") == 0) return "text/html";
    else if (strcmp(ext, ".ico") == 0) return "image/vnd.microsoft.icon";
    else if (strcmp(ext, ".ics") == 0) return "text/calendar";
    else if (strcmp(ext, ".jar") == 0) return "application/java-archive";
    else if (strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".jpg") == 0) return "image/jpeg";
    else if (strcmp(ext, ".js") == 0) return "text/javascript";
    else if (strcmp(ext, ".json") == 0) return "application/json";
    else if (strcmp(ext, ".jsonld") == 0) return "application/ld+json";
    else if (strcmp(ext, ".mid") == 0 || strcmp(ext, ".midi") == 0) return "audio/midi";
    else if (strcmp(ext, ".mjs") == 0) return "text/javascript";
    else if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    else if (strcmp(ext, ".mp4") == 0) return "video/mp4";
    else if (strcmp(ext, ".mpeg") == 0) return "video/mpeg";
    else if (strcmp(ext, ".mpkg") == 0) return "application/vnd.apple.installer+xml";
    else if (strcmp(ext, ".odp") == 0) return "application/vnd.oasis.opendocument.presentation";
    else if (strcmp(ext, ".ods") == 0) return "application/vnd.oasis.opendocument.spreadsheet";
    else if (strcmp(ext, ".odt") == 0) return "application/vnd.oasis.opendocument.text";
    else if (strcmp(ext, ".oga") == 0) return "audio/ogg";
    else if (strcmp(ext, ".ogv") == 0) return "video/ogg";
    else if (strcmp(ext, ".ogx") == 0) return "application/ogg";
    else if (strcmp(ext, ".opus") == 0) return "audio/ogg";
    else if (strcmp(ext, ".otf") == 0) return "font/otf";
    else if (strcmp(ext, ".png") == 0) return "image/png";
    else if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    else if (strcmp(ext, ".php") == 0) return "application/x-httpd-php";
    else if (strcmp(ext, ".ppt") == 0) return "application/vnd.ms-powerpoint";
    else if (strcmp(ext, ".pptx") == 0) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    else if (strcmp(ext, ".rar") == 0) return "application/vnd.rar";
    else if (strcmp(ext, ".rtf") == 0) return "application/rtf";
    else if (strcmp(ext, ".sh") == 0) return "application/x-sh";
    else if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    else if (strcmp(ext, ".tar") == 0) return "application/x-tar";
    else if (strcmp(ext, ".tif") == 0 || strcmp(ext, ".tiff") == 0) return "image/tiff";
    else if (strcmp(ext, ".ts") == 0) return "video/mp2t";
    else if (strcmp(ext, ".ttf") == 0) return "font/ttf";
    else if (strcmp(ext, ".txt") == 0) return "text/plain";
    else if (strcmp(ext, ".vsd") == 0) return "application/vnd.visio";
    else if (strcmp(ext, ".wav") == 0) return "audio/wav";
    else if (strcmp(ext, ".weba") == 0) return "audio/webm";
    else if (strcmp(ext, ".webm") == 0) return "video/webm";
    else if (strcmp(ext, ".webp") == 0) return "image/webp";
    else if (strcmp(ext, ".woff") == 0) return "font/woff";
    else if (strcmp(ext, ".woff2") == 0) return "font/woff2";
    else if (strcmp(ext, ".xhtml") == 0) return "application/xhtml+xml";
    else if (strcmp(ext, ".xls") == 0) return "application/vnd.ms-excel";
    else if (strcmp(ext, ".xlsx") == 0) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    else if (strcmp(ext, ".xml") == 0) return "application/xml";
    else if (strcmp(ext, ".xul") == 0) return "application/vnd.mozilla.xul+xml";
    else if (strcmp(ext, ".zip") == 0) return "application/zip";
    else if (strcmp(ext, ".3gp") == 0) return "video/3gpp";
    else if (strcmp(ext, ".3g2") == 0) return "video/3gpp2";
    else if (strcmp(ext, ".7z") == 0) return "application/x-7z-compressed";

    return "application/octet-stream";
}

void handle_file_request(int client_fd, const char *file, int flags){
    char path[256];

    snprintf(path, sizeof(path), "%s/%s", flags ? ROUTES : CATCHALL, file);

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

    http_request req = {0};
    parse_http_req(buffer, &req);

    if(!validate_request(&req)){
        printf("Invalid HTTP request.");
        handle_not_found(client_fd);
        return;
    }

    route *current_route = server.route;
    while (current_route != NULL) {
        if (match_route(req.path, current_route->path) && strcmp(req.method, current_route->method) == 0) {
            current_route->callback(client_fd, &req);
            close(client_fd);
            return;
        }
        current_route = current_route->next;
    }

    if(strcmp(req.method, "GET") == 0){
        handle_file_request(client_fd, req.path+1 , 0);
    }

    handle_not_found(client_fd);
}


void free_routes() {
    route *current = server.route;
    while (current != NULL) {
        route *next = current->next;
        free(current);
        current = next;
    }
}

void handle_sigint(int sig) {
    printf("\nShutting down sever...\n");
    if(server.sckt) {
        close(server.sckt);
    }
    free_routes();
    exit(0);
}

int main(){
    signal(SIGINT, (void (*)(int))handle_sigint);

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
        handle_error("setsockopt failed.", sckt);
    }

    if (bind(sckt, (struct sockaddr*)&addr, sizeof(addr)) != 0){
        handle_error("Bind failed.", sckt);
    };

    if(listen(sckt, 10) != 0){
        handle_error("Listen failed.", sckt);
    };

    printf("Server listening on port %d\n", PORT);

    load_routes();
    printf("Routes loaded.\n\n");
    print_routes();

    while(1){
        int client_fd = accept(sckt, NULL, NULL);
        if (client_fd < 0) {
            perror("Accept failed.");
            continue;
        }
        handle_client(client_fd);
    }

    close(sckt);
    return 0;
}
