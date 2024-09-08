#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

extern route *routes;

void add_route(const char *method, const char *path, void (*callback)(int client_fd, http_request *req)){
    route *r = malloc(sizeof(route));
    if(r == NULL) {
        perror("Failed to allocate memory");
        return;
    }
    strncpy(r->method, method, sizeof(r->method) - 1);
    strncpy(r->path, path, sizeof(r->path) - 1);
    r->callback = callback;
    r->next = routes;
    routes = r;
}

void print_routes() {
    route *current = routes;
    while (current != NULL) {
        printf("Method: %s, Path: %s\n", current->method, current->path);
        current = current->next;
    }
}

void handle_example(int client_fd, http_request *req){
    handle_file_request(client_fd, "hello/index.html", 1);
}

void load_routes() {
    add_route("GET", "/hello", handle_example);
    add_route("GET", "/", handle_example);
}
