#include <signal.h>
#include <sys/epoll.h>
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
#include "db.h"

Server server;
MimeEntry mime_types[] = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".gif", "image/gif"},
    {".txt", "text/plain"},
    {".json", "application/json"},
    {".svg", "image/svg+xml"},
    {".pdf", "application/pdf"},
};

void handle_critical_error(char *msg, int sckt)
{
    perror(msg);
    if (sckt > 0)
        close(sckt);
    exit(EXIT_FAILURE);
}

ErrorInfo get_error_info(ErrorType err)
{
    switch (err)
    {
    case ERR_NOTFOUND:
        return (ErrorInfo){404, "File Not Found."};
    case ERR_BADREQ:
        return (ErrorInfo){400, "Bad Request."};
    case ERR_INTERR:
        return (ErrorInfo){500, "Internal Server Error."};
    default:
        return (ErrorInfo){500, "Unknown Error."};
    }
}

int set_non_blocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void send_error_response(int client_fd, int error_type)
{
    ErrorInfo error_info = get_error_info(error_type);

    char body[512];
    snprintf(body, sizeof(body), "<html><body><h1>%d %s</h1></body></html>", error_info.status_code, error_info.message);

    char response[1024];
    snprintf(response, sizeof(response), "HTTP/1.1 %d %s\r\nContent-Type: text/html\r\nContent-Length: %zu\r\n\r\n%s",
             error_info.status_code, error_info.message, strlen(body), body);
    send(client_fd, response, strlen(response), 0);
}

//TODO: redo
void send_string(int client_fd, char *str)
{
    char response[4096];
    snprintf(response, sizeof(response), "HTTP/1.1 %d %s\r\nContent-Type: text/html\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
             200, "OK", strlen(str), str);
    send(client_fd, response, strlen(response), 0);
}

void send_json_response(int client_fd, char *json)
{
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", strlen(json), json);
    send(client_fd, response, strlen(response), 0);
}

int validate_request(const HttpRequest *req)
{
    return strlen(req->path) > 0 && strlen(req->method) > 0 && strlen(req->version) > 0;
}

void handle_parsing_error(int client_fd, char *buf, HttpRequest *req, ErrorType error_type)
{
    free(buf);
    free_http_req(req);
    send_error_response(client_fd, error_type);
}

int parse_http_req(int client_fd, const char *buffer, HttpRequest *http_req)
{
    char *buf = strdup(buffer);

    if (!buf)
    {
        perror("Memory allocation failed. (parse_http_req)");
        handle_parsing_error(client_fd, buf, http_req, ERR_INTERR);
        return -1;
    }

    char *line_end = strstr(buf, "\r\n");
    if (!line_end)
    {
        handle_parsing_error(client_fd, buf, http_req, ERR_BADREQ);
        return -2;
    }

    *line_end = '\0';

    char *method_end = strchr(buf, ' ');
    if (!method_end)
    {
        handle_parsing_error(client_fd, buf, http_req, ERR_BADREQ);
        return -2;
    }

    *method_end = '\0';
    http_req->method = strdup(buf);
    if (!http_req->method)
    {
        perror("Mem allocation failed. (method)");
        handle_parsing_error(client_fd, buf, http_req, ERR_INTERR);
        return -1;
    }

    char *path_start = method_end + 1;
    char *path_end = strchr(path_start, ' ');
    if (!path_end)
    {
        handle_parsing_error(client_fd, buf, http_req, ERR_BADREQ);
        return -2;
    }

    *path_end = '\0';
    http_req->path = strdup(path_start);
    if (!http_req->path)
    {
        perror("Mem allocation failed. (path)");
        handle_parsing_error(client_fd, buf, http_req, ERR_INTERR);
        return -1;
    }

    char *version_start = path_end + 1;
    http_req->version = strdup(version_start);
    if (!http_req->version)
    {
        perror("Memory allocation failed. (version)");
        handle_parsing_error(client_fd, buf, http_req, ERR_INTERR);
        return -1;
    }

    http_req->headers_len = 0;
    http_req->headers = calloc(MAX_HEADERS, sizeof(Header));

    if (!http_req->headers)
    {
        perror("Memory allocation failed. (headers)");
        handle_parsing_error(client_fd, buf, http_req, ERR_INTERR);
        return -1;
    }

    char *header_line = line_end + 2;
    char *body_start = NULL;

    while (*header_line && http_req->headers_len < MAX_HEADERS)
    {
        char *header_end = strstr(header_line, "\r\n");
        if (!header_end)
            break;

        *header_end = '\0';

        if (header_line == header_end)
        {
            body_start = header_end + 2;
            break;
        }

        char *colon = strchr(header_line, ':');
        if (!colon)
        {
            header_line += 2;
            continue;
        }

        *colon = '\0';
        char *header_name = header_line;
        char *header_value = colon + 1;

        while (*header_value == ' ')
            header_value++;

        http_req->headers[http_req->headers_len].name = strdup(header_name);
        http_req->headers[http_req->headers_len].value = strdup(header_value);

        if (!http_req->headers[http_req->headers_len].name || !http_req->headers[http_req->headers_len].value)
        {
            perror("Memory allocation failed. (header fields)");
            header_end += 2;
            continue;
        }

        http_req->headers_len++;
        header_line = header_end + 2;
    }

    if (body_start)
    {
        http_req->body = strdup(body_start);
        if (!http_req->body)
        {
            perror("Memory allocation failed. (body)");
            handle_parsing_error(client_fd, buf, http_req, ERR_INTERR);
            return -1;
        }
    }

    free(buf);
    return 0;
}

void free_http_req(HttpRequest *req)
{
    free(req->method);
    free(req->path);
    free(req->version);

    for (int i = 0; i < req->headers_len; i++)
    {
        free(req->headers[i].name);
        free(req->headers[i].value);
    }
    free(req->headers);
    free(req->body);
}

const char *get_mime_type(const char *path)
{
    const char *ext = strchr(path, '.');
    if (!ext)
        return "application/octet-stream";

    for (int i = 0; i < (int) (sizeof(mime_types) / sizeof(mime_types[0])); i++)
    {
        if (strcmp(mime_types[i].extension, ext) == 0)
        {
            return mime_types[i].mime_type;
        }
    }

    return "application/octet-stream";
}

int serve_file(int client_fd, const char *path)
{
    char p[512];

    snprintf(p, 512, "%s/%s", get_routes_dir(), path);
    int file_fd = open(p, O_RDONLY);

    if (file_fd == -1)
    {
        snprintf(p, 512, "%s/%s", get_public_dir(), path);
        file_fd = open(p, O_RDONLY);
        LOG("%s", p);

        if (file_fd == -1)
        {
            LOG("Not found");
            send_error_response(client_fd, ERR_NOTFOUND);
            return -1;
        }
    }

    struct stat st;
    if (fstat(file_fd, &st) == -1)
    {
        close(file_fd);
        send_error_response(client_fd, ERR_INTERR);
        return -1;
    }

    char buffer[BUFFER_SIZE];
    const char *mime_type = get_mime_type(path);
    snprintf(buffer, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: %s\r\n"
                                  "Content-Length: %ld\r\n" \
                                  "Connection: close\r\n\r\n",
             mime_type, st.st_size);

    send(client_fd, buffer, strlen(buffer), 0);

    ssize_t offset = 0;
    sendfile(client_fd, file_fd, &offset, st.st_size);

    close(file_fd);
    return 0;
}

void handle_client(int client_fd)
{
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_recieved = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes_recieved < 0)
    {
        LOG("RECV failed.");
        return;
    }
    buffer[bytes_recieved] = '\0';

    HttpRequest req = {0};
    int parse_result = parse_http_req(client_fd, buffer, &req);
    if (parse_result != 0)
    {
        return;
    }

    if (!validate_request(&req))
    {
        send_error_response(client_fd, ERR_BADREQ);
        free_http_req(&req);
        return;
    }

    for (Route *r = server.route; r; r = r->next)
    {
        if (strcmp(req.method, r->method) == 0 && match_route(req.path, r->path))
        {
            r->callback(client_fd, &req);
            free_http_req(&req);
            return;
        }
    }

    if (strcmp(req.method, "GET") == 0)
    {
        serve_file(client_fd, req.path + 1);
    }

    send_error_response(client_fd, ERR_NOTFOUND);
    free_http_req(&req);
}

void handle_sigint(int sig)
{
    LOG("Shutting down server... (%d)", sig);
    if (server.sckt)
    {
        close(server.sckt);
    }
    free_routes(server.route);
    exit(0);
}

int main()
{
    load_env(".env");
    signal(SIGINT, handle_sigint);

    const int PORT = get_port();
    struct epoll_event ev, events[MAX_EVENTS];

    int sckt = socket(AF_INET, SOCK_STREAM, 0);
    if (sckt < 0)
        handle_critical_error("Socket creation falied.", -1);
    server.sckt = sckt;
    server.route = NULL;

    set_non_blocking(sckt);

    const struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr = {INADDR_ANY},
        .sin_zero = {0},
    };

    int opt = 1;
    if (setsockopt(sckt, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        handle_critical_error("setsockopt failed.", server.sckt);
    }

    if (bind(sckt, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        handle_critical_error("Bind failed.", server.sckt);
    };

    if (listen(sckt, SOMAXCONN) != 0)
    {
        handle_critical_error("Listen failed.", server.sckt);
    };

    int epoll_fd;
    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1){
        handle_critical_error("epoll_create1 failed.", epoll_fd);
    }

    ev.events = EPOLLIN;
    ev.data.fd = sckt;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sckt, &ev);

    printf("Server listening on port %d\n", PORT);

    load_routes();
    printf("Routes loaded.\n\n");
    print_routes();

    while (1)
    {
        int num_fds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for(int i = 0; i < num_fds; i++){
            if(events[i].data.fd == sckt){
                int client_fd = accept(server.sckt, NULL, NULL);
                set_non_blocking(client_fd);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
            }
            else{
                int client_fd = events[i].data.fd;
                handle_client(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                close(client_fd);
            }
        }
    }

    close(sckt);
    close(epoll_fd);
    return 0;
}
