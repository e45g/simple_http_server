#ifndef UTILS_H
#define UTILS_H

#include "json/json.h"

void logg(long line, const char *file, const char *func, const char *format, ...);
#define LOG(format, ...) logg(__LINE__, __FILE__, __PRETTY_FUNCTION__, format, ##__VA_ARGS__)
#define MAX_LINE_LENGTH 256
#define MIN_COMPRESSION_THRESHOLD 4096

int load_env(const char *path);
int get_port(void);
const char *get_routes_dir(void);
const char *get_public_dir(void);

void generate_id(char *buffer);
void get_current_time(char *buffer, size_t size, long offset);

char *compress_data(const char *json, size_t json_len, size_t *compressed_len);

#endif
