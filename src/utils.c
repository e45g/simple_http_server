#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "utils.h"
#include "lib/cJSON/cJSON.h"

void logg(long line, const char *file, const char *func, const char *format, ...){
    printf("LOG: [%s:%ld %s] ", file, line, func);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}

int load_env(const char *path){
    FILE *f = fopen(path, "r");
    if(!f){
        LOG("Invalid .env file");
        return -1;
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), f)) {
        *(strchr(line, '\n')) = '\0';

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");

        if(key && value){
            setenv(key, value, 1);
        }

    }
    return 1;
}

int get_port(){
    const char *port = getenv("PORT");
    return port ? strtol(port, NULL, 10) : 1444;
}

const char *get_routes_dir(){
    const char *dir = getenv("ROUTES_DIR");
    return dir ? dir : "./routes";
}

const char *get_public_dir(){
    const char *dir = getenv("PUBLIC_DIR");
    return dir ? dir : "./public";
}

const char *cjson_get_string(cJSON *json, char *key){
    cJSON *value = cJSON_GetObjectItemCaseSensitive(json, key);

    if(value->valuestring != NULL && cJSON_IsString(value)){
        return value->valuestring;
    }

    return NULL;
}

int cjson_get_number(cJSON *json, char *key){
    cJSON *value = cJSON_GetObjectItemCaseSensitive(json, key);

    if(cJSON_IsNumber(value)){
        return cJSON_GetNumberValue(value);
    }

    return 0;
}
