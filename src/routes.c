#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "db.h"
#include "server.h"
#include "backend/api.h"
#include "utils.h"

extern Server server;

int match_route(const char *route, const char *handle) {
    const char *r = route;
    const char *h = handle;
    while (*r && *h) {
        if (*h == '*') {
            r = strchr(r, '/');
            if (!r) {
                return 1;
            }
            h++;
            continue;
        }
        else if (*h != *r) {
            return 0;
        }
        h++;
        r++;
    }

    return (*r == '\0' && (*h == '\0' || *h == '*'));
}

void get_wildcards(HttpRequest *req, const Route *r){
    const char *req_path = req->path;
    const char *route_path = r->path;

    int req_len = strlen(req_path);
    int route_len = strlen(route_path);

    int i = 0;
    int j = 0;

    req->wildcard_num = 0;

    while(i < req_len && j < route_len){
        if(route_path[j] == '*'){
            j++;
            if(req->wildcard_num < 16){
                int k = 0;
                while(i < req_len && req_path[i] != '/' && k < 63){
                    req->wildcards[req->wildcard_num][k++] = req_path[i++];
                }
                req->wildcards[req->wildcard_num][k] = '\0';
                req->wildcard_num++;
            }
        }
        i++;
        j++;
    }
}

void add_route(const char *method, const char *path, void (*callback)(int client_fd, HttpRequest *req)) {
    Route *r = malloc(sizeof(Route));
    if (r == NULL) {
        LOG("Failed to allocate memory");
        return;
    }
    strncpy(r->method, method, sizeof(r->method) - 1);
    strncpy(r->path, path, sizeof(r->path) - 1);
    r->callback = callback;
    r->next = server.route;
    server.route = r;
}

void print_routes(void) {
    for (Route *r = server.route; r; r = r->next)
    {
        LOG("Route - %s: %s", r->method, r->path);
    }
}

void free_routes(void) {
    Route *current = server.route;
    while (current)
    {
        Route *tmp = current->next;
        free(current);
        current = tmp;
    }
}

void handle_root(int client_fd, HttpRequest *req __attribute__((unused))) {
    send_string(client_fd, "Hello TdA");
}

void handle_hello(int client_fd, HttpRequest *req __attribute__((unused))) {
    serve_file(client_fd, "test/a.html");
}
void handle_test(int client_fd, HttpRequest *req __attribute__((unused))) {
    char date[64];
    get_current_time(date, 64, -300);

    const char *params[] = {
        date
    };

    DBResult *result = db_query("SELECT * FROM games WHERE created_at >= ?", params, 1);
    if(!result) return;
    printf("%d %d\n", result->num_cols, result->num_rows);
    for(int i = 0; i < result->num_rows; i++){
        for(int j = 0; j < result->num_cols; j++){
            printf("%s ", result->rows[i][j]);
        }
        printf("\n");
    }
    free_result(result);

    send_string(client_fd, "look at em");
}

void handle_log(int client_fd, HttpRequest *req __attribute__((unused))) {
    serve_file(client_fd, "../log.txt");
}

void load_routes(void) {
    add_route("GET", "/", handle_root);
    add_route("GET", "/api", handle_api);

    add_route("POST", "/api/v1/games", handle_game_creation);
    add_route("PUT", "/api/v1/games/*", handle_game_update);
    add_route("DELETE", "/api/v1/games/*", handle_game_deletion);
    add_route("GET", "/api/v1/games/*", handle_get_game);
    add_route("GET", "/api/v1/games", handle_list_games);

    add_route("GET", "/test", handle_test);
    add_route("GET", "/game", handle_hello);
    add_route("GET", "/game/*", handle_hello);

    add_route("GET", "/log", handle_log);

}
