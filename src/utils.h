#ifndef UTILS_H
#define UTILS_H

#include "lib/cJSON/cJSON.h"

void logg(long line, const char *file, const char *func, const char *format, ...);
#define LOG(format, ...) logg(__LINE__, __FILE__, __PRETTY_FUNCTION__, format, ##__VA_ARGS__)
#define MAX_LINE_LENGTH 256

int load_env(const char *path);
int get_port();
const char *get_routes_dir();
const char *get_public_dir();

const char *cjson_get_string(cJSON *json, char *key);
int cjson_get_number(cJSON *json, char *key);

#endif
