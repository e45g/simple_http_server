#ifndef ROUTES_H
#define ROUTES_H

#include "server.h"

int match_route(char *route, char *handle);
void add_route(const char *method, const char *path, void (*callback)(int client_fd, http_request *req));
void load_routes();
void print_routes();

#endif
