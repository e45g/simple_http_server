#ifndef ROUTES_H
#define ROUTES_H

typedef struct {
    char method[16];
    char path[265];
    int client_fd;
    void (*func)(int);
} route;

#endif
