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
int sckt;

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
    if (strcmp(ext, ".abw") == 0) return "application/x-abiword";
    if (strcmp(ext, ".apng") == 0) return "image/apng";
    if (strcmp(ext, ".arc") == 0) return "application/x-freearc";
    if (strcmp(ext, ".avif") == 0) return "image/avif";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".azw") == 0) return "application/vnd.amazon.ebook";
    if (strcmp(ext, ".bin") == 0) return "application/octet-stream";
    if (strcmp(ext, ".bmp") == 0) return "image/bmp";
    if (strcmp(ext, ".bz") == 0) return "application/x-bzip";
    if (strcmp(ext, ".bz2") == 0) return "application/x-bzip2";
    if (strcmp(ext, ".cda") == 0) return "application/x-cdf";
    if (strcmp(ext, ".csh") == 0) return "application/x-csh";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".csv") == 0) return "text/csv";
    if (strcmp(ext, ".doc") == 0) return "application/msword";
    if (strcmp(ext, ".docx") == 0) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (strcmp(ext, ".eot") == 0) return "application/vnd.ms-fontobject";
    if (strcmp(ext, ".epub") == 0) return "application/epub+zip";
    if (strcmp(ext, ".gz") == 0) return "application/gzip";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".htm") == 0 || strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".ico") == 0) return "image/vnd.microsoft.icon";
    if (strcmp(ext, ".ics") == 0) return "text/calendar";
    if (strcmp(ext, ".jar") == 0) return "application/java-archive";
    if (strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".jpg") == 0) return "image/jpeg";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".jsonld") == 0) return "application/ld+json";
    if (strcmp(ext, ".mid") == 0 || strcmp(ext, ".midi") == 0) return "audio/midi";
    if (strcmp(ext, ".mjs") == 0) return "application/javascript";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    if (strcmp(ext, ".mp4") == 0) return "video/mp4";
    if (strcmp(ext, ".mpeg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mpkg") == 0) return "application/vnd.apple.installer+xml";
    if (strcmp(ext, ".odp") == 0) return "application/vnd.oasis.opendocument.presentation";
    if (strcmp(ext, ".ods") == 0) return "application/vnd.oasis.opendocument.spreadsheet";
    if (strcmp(ext, ".odt") == 0) return "application/vnd.oasis.opendocument.text";
    if (strcmp(ext, ".oga") == 0) return "audio/ogg";
    if (strcmp(ext, ".ogv") == 0) return "video/ogg";
    if (strcmp(ext, ".ogx") == 0) return "application/ogg";
    if (strcmp(ext, ".opus") == 0) return "audio/ogg";
    if (strcmp(ext, ".otf") == 0) return "font/otf";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    if (strcmp(ext, ".php") == 0) return "application/x-httpd-php";
    if (strcmp(ext, ".ppt") == 0) return "application/vnd.ms-powerpoint";
    if (strcmp(ext, ".pptx") == 0) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    if (strcmp(ext, ".rar") == 0) return "application/vnd.rar";
    if (strcmp(ext, ".rtf") == 0) return "application/rtf";
    if (strcmp(ext, ".sh") == 0) return "application/x-sh";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".tar") == 0) return "application/x-tar";
    if (strcmp(ext, ".tif") == 0 || strcmp(ext, ".tiff") == 0) return "image/tiff";
    if (strcmp(ext, ".ts") == 0) return "video/mp2t";
    if (strcmp(ext, ".ttf") == 0) return "font/ttf";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".vsd") == 0) return "application/vnd.visio";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".weba") == 0) return "audio/webm";
    if (strcmp(ext, ".webm") == 0) return "video/webm";
    if (strcmp(ext, ".webp") == 0) return "image/webp";
    if (strcmp(ext, ".woff") == 0) return "font/woff";
    if (strcmp(ext, ".woff2") == 0) return "font/woff2";
    if (strcmp(ext, ".xhtml") == 0) return "application/xhtml+xml";
    if (strcmp(ext, ".xls") == 0) return "application/vnd.ms-excel";
    if (strcmp(ext, ".xlsx") == 0) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (strcmp(ext, ".xml") == 0) return "application/xml";
    if (strcmp(ext, ".xul") == 0) return "application/vnd.mozilla.xul+xml";
    if (strcmp(ext, ".zip") == 0) return "application/zip";
    if (strcmp(ext, ".3gp") == 0) return "video/3gpp";
    if (strcmp(ext, ".3g2") == 0) return "video/3gpp2";
    if (strcmp(ext, ".7z") == 0) return "application/x-7z-compressed";

    return "application/octet-stream";
}

void handle_file_request(int client_fd, const char *file, int flags){
    char path[256];

    snprintf(path, sizeof(path), "%s/%s", flags ? FRONTEND : CATCHALL, file);

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

    if(strcmp(req.method, "GET") == 0){
        handle_file_request(client_fd, req.path+1 , 0);
    }

    handle_not_found(client_fd);
}

void free_routes() {
    route *current = routes;
    while (current != NULL) {
        route *next = current->next;
        free(current);
        current = next;
    }
    routes = NULL;
}


void handle_sigint(int sig) {
    printf("\nShutting down sever...\n");
    if(sckt) {
        close(sckt);
    }
    free_routes();
    exit(0);
}

int main(){
    signal(SIGINT, handle_sigint);
	setbuf(stdout, NULL);
    sckt = socket(AF_INET, SOCK_STREAM, 0);
    if (sckt < 0){
        perror("Socket failed.");
        exit(EXIT_FAILURE);
    }

    const struct sockaddr_in addr = {
        AF_INET,
        htons(PORT),
        0,
    };

    int opt = 1;
    if (setsockopt(sckt, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed.");
        close(sckt);
        exit(EXIT_FAILURE);
    }

    if (bind(sckt, (struct sockaddr*)&addr, sizeof(addr)) != 0){
        perror("Bind failed.");
        close(sckt);
        exit(EXIT_FAILURE);
    };

    if(listen(sckt, 10) != 0){
        perror("Listen failed.");
        close(sckt);
        exit(EXIT_FAILURE);
    };

    printf("Server listening on port %d\n", PORT);

    load_routes();
    printf("Routes loaded.\n\n");
    print_routes();

    while(1){
        int client_fd = accept(sckt, NULL, NULL);
        if (client_fd < 0) {
            perror("Accept failed.");
            close(sckt);
            continue;
        }
        handle_client(client_fd);
    }

    close(sckt);
    return 0;
}
