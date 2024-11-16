#ifndef ROUTES_H
#define ROUTES_H

#include "server.h"

int match_route(char *route, char *handle);
void get_wildcards(const HttpRequest *req, const Route *r);
void add_route(const char *method, const char *path, void (*callback)(int client_fd, HttpRequest *req));
void free_routes(Route *route);
void load_routes(void);
void print_routes(void);

#endif
