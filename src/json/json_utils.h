#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "json.h"

char *escape_string(const char *str);
void json_to_string(const Json *json, char **buffer, size_t *pos, size_t *max_size);
int ensure_buffer_size(char **buffer, size_t *max_size, size_t required_space);

double parse_number(const char **str);
Json *parse_object(const char **str);
char *parse_string(const char **str);
Json *parse_value(const char **str);
Json *parse_array(const char **str);

#endif
