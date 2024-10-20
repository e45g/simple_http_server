#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "lib/cJSON/cJSON.h"
#include "utils.h"

#include "cxc/layout.h"

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

void add_route(const char *method, const char *path, void (*callback)(int client_fd, HttpRequest *req)) {
    Route *r = malloc(sizeof(Route));
    if (r == NULL) {
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
    for (Route *r = server.route; r; r = r->next)
    {
        printf("Route - %s: %s\n", r->method, r->path);
    }
}

void free_routes() {
    Route *current = server.route;
    while (current)
    {
        Route *tmp = current->next;
        free(current);
        current = tmp;
    }
}

void handle_example(int client_fd, HttpRequest *req __attribute__((unused))) {
    serve_file(client_fd, "example/index.html");
}

void handle_cxc_example(int client_fd, HttpRequest *req __attribute__((unused))) {
    LayoutProps props = {0};
    char *output = render_layout(&props);
    send_string(client_fd, output);
    free(output);
}

void handle_post(int client_fd, HttpRequest *req) {
    cJSON *json = cJSON_Parse(req->body);

    int status = cjson_get_number(json, "s");

    char response[256];
    snprintf(response, 256, "{\"status\": %d}", status);

    send_json_response(client_fd, response);
    cJSON_Delete(json);
}

void load_routes() {
    add_route("GET", "/", handle_example);
    add_route("GET", "/cxc", handle_cxc_example);
    add_route("POST", "/post_test", handle_post);
}
