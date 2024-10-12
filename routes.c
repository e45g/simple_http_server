#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

extern server_t server;

int match_route(char *route, char *handle){
    char *route_cpy = strdup(route);
    char *handle_cpy = strdup(handle);
    char *saveptr1, *saveptr2;
    char *r, *h;

    r = strtok_r(route_cpy, "/", &saveptr1);
    h = strtok_r(handle_cpy, "/", &saveptr2);


    while(r != NULL && h != NULL){
        if(strcmp(h, "*") != 0){
            if(strcmp(r, h) != 0){
                free(route_cpy);
                free(handle_cpy);
                return 0;
            }
        }
        r = strtok_r(NULL, "/", &saveptr1);
        h = strtok_r(NULL, "/", &saveptr2);
    }

    int result = (r == NULL && h == NULL);

    free(route_cpy);
    free(handle_cpy);

    return result;
}

void add_route(const char *method, const char *path, void (*callback)(int client_fd, http_request *req)){
    route *r = malloc(sizeof(route));
    if(r == NULL) {
        perror("Failed to allocate memory");
        return;
    }
    strncpy(r->method, method, sizeof(r->method) - 1);
    strncpy(r->path, path, sizeof(r->path) - 1);
    r->callback = callback;
    r->next = server.route;
    server.route = r;
}

void print_routes() {
    route *current = server.route;
    while (current != NULL) {
        printf("Method: %s, Path: %s\n", current->method, current->path);
        current = current->next;
    }
}

void handle_example(int client_fd, http_request *req){
    printf("%s\n", req->path);
    handle_file_request(client_fd, "hello/index.html", 1);
}

void load_routes() {
    add_route("GET", "/hello", handle_example);
    add_route("GET", "/", handle_example);
    add_route("GET", "/abc/*", handle_example);
}
