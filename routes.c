#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"

extern Server server;

int match_route(const char *route, const char *handle){
    while(*route && *handle){
        if(*handle == '*'){
            route = strchr(route, '/');
            handle++;
            continue;
        }
        else if(*handle != *route){
            return 0;
        }
        handle++;
        route++;
    }

    return (*route == '\0' && (*handle == '\0' || *handle == '*'));
}

void add_route(const char *method, const char *path, void (*callback)(int client_fd, HttpRequest *req)){
    Route *r = malloc(sizeof(Route));
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
    for(Route *r = server.route; r; r = r->next){
        printf("Route - %s: %s\n", r->method, r->path);
    }
}

void free_routes(Route *route) {
    for(Route *r = route; r; r = r->next){
        free(r);
    }
}

void handle_example(int client_fd, HttpRequest *req){
    serve_file(client_fd, "hello/index.html");
}

void handle_post(int client_fd, HttpRequest *req){

}


void load_routes() {
    add_route("GET", "/", handle_example);
    add_route("GET", "/hello", handle_example);
    add_route("GET", "/abc/*", handle_example);
    add_route("POST", "/post_test", handle_post);
}
