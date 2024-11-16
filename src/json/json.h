#ifndef JSON_H
#define JSON_H

#include <stddef.h>

/*
*   TODO:
*   FIX double free problem when adding same Json* to object or array
*
*/

enum JsonType{
    JSON_NULL,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
};

typedef struct Json{
    enum JsonType type;
    union {
        double number;
        char *string;

        struct {
            struct Json **elements;
            size_t size;
            size_t capacity;
        } array;

        struct {
            char *key;
            struct Json *value;
            struct Json *next;
        } object;

    } value;
} Json;

Json *json_create_object();
Json *json_create_array(size_t initial_capacity);
Json *json_create_string(const char *string);
Json *json_create_number(double number);
Json *json_create_null();
Json *json_create_true();
Json *json_create_false();

Json *json_parse(const char *json_str);
char *json_print(Json *json);

int json_array_add(Json *array, Json *value);
// int json_array_remove(Json *array, size_t index); // TODO
Json *json_array_get(Json *array, size_t index);

int json_object_add(Json *object, const char *key, Json *value);
int json_object_add_string(Json *json, const char *key, const char *value);
// int json_object_remove(Json *object, const char *key); // TODO
Json *json_object_get(Json *object, const char *key);
char *json_object_get_string(Json *object, const char *key);
double *json_object_get_number(Json *object, const char *key);
Json *json_object_get_array(Json *object, const char *key);


int json_is_string(Json *json);
int json_is_array(Json *json);
int json_is_object(Json *json);

void json_free(Json *json);

#endif
