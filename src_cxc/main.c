#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>

#define CTEMPLATE_PATH "./src_cxc/ctemplate.c"
#define HTEMPLATE_PATH "./src_cxc/htemplate.h"

#define SAVE_PATH "./src/cxc/"

#define PATH_TO_CX_FILES "./test"
#define MAX_NAME 64
#define MAX_FILE_NAME 128
#define BUFFER_SIZE 4096

unsigned long response_length = 0;

typedef enum {
    C,
    H
} Template;

typedef struct {
    char placeholder[128];
    char *replacement;
} Placeholder;

int replace_placeholder(char **buffer, const char *placeholder, const char *replacement) {
    ssize_t buffer_len = strlen(*buffer);
    ssize_t placeholder_len = strlen(placeholder);
    ssize_t replacement_len = strlen(replacement);

    char *pos = strstr(*buffer, placeholder);

    while (pos != NULL) {
        ssize_t new_len = buffer_len - placeholder_len + replacement_len;
        if (new_len >= BUFFER_SIZE) {
            char *new_buffer = realloc(*buffer, new_len + 1);
            if (new_buffer == NULL) {
                perror("Error allocating memory");
                return -1;
            }
            *buffer = new_buffer;
        }

        memmove(pos + replacement_len, pos + placeholder_len, buffer_len - (pos - *buffer) - placeholder_len + 1);
        memcpy(pos, replacement, replacement_len);

        buffer_len = new_len;
        pos = strstr(*buffer, placeholder);
    }

    return 0;
}

int get_props_name(char *dst, const char *filename){
    char tmp[MAX_NAME];
    strcpy(tmp, filename);

    char *token = strtok(tmp, "_");
    while(token != NULL){
        *token = toupper(*token);
        strcat(dst, token);
        token = strtok(NULL, "_");
    }
    strcat(dst, "Props");

    return 0;
}

int has_cx_extension(const char *filename){
    const char *dot = strrchr(filename, '.');
    return (dot && strcmp(".cx", dot) == 0);
}

int get_file_length(FILE *f){
    fseek(f, 0L, SEEK_END);
    long length = ftell(f);
    fseek(f, 0L, SEEK_SET);
    return length;
}

void process_text(char *s) {
    char *p = s;
    while(*s != '\0'){
        if(*s == '\n'){
            memmove(s, s+1, strlen(s+1) + 1);

        }
        else if(*s == '"'){
            memmove(s+2, s+1, strlen(s) + 1);
            *s = '\\';
            *(++s) = '"';
        }
        s++;
    }
    s = p;
}

void process_code(char *s){
    char tmp[BUFFER_SIZE] = {0};
    if(*s == '='){

        snprintf(tmp, BUFFER_SIZE, "\tstrcat(output, %s);", s+1);
    }
    else{
        snprintf(tmp, BUFFER_SIZE, "\t%s", s);
    }

    strcpy(s, tmp);
    strcat(s, "\n");
}

char *get_template(Template t){
   FILE *f = fopen(t ? HTEMPLATE_PATH : CTEMPLATE_PATH, "r");
   if(f == NULL) {
        perror("Could not open template.");
        return NULL;
    }
    long length = get_file_length(f);
    if(length <= 0) {
        perror("Length 0 template.");
        return NULL;
    }
    char *content = calloc(sizeof(char)*length+1, sizeof(char));
    if(content == NULL){
        perror("Error allocating memory.");
        return NULL;
    }
    fread(content, 1, length, f);
    return content;
}

int save_file(char *path, char *buf){
    FILE *f = fopen(path, "w");
    if(f == NULL){
        perror("Error opening file (w).");
        return -1;
    }

    fwrite(buf, 1, strlen(buf), f);

    return 0;
}

int generate(FILE *f, const char *filename, long length, char *ctemp, char *htemp){
    response_length = 0;
    char *content;
    char *ptr;
    char props_name[MAX_NAME] = {0};
    char main_function_name[MAX_NAME] = {0};
    char file_id[MAX_NAME] = {0};

    char props_struct[BUFFER_SIZE] = {0};
    char prepend[BUFFER_SIZE] = {0};
    char function_code[BUFFER_SIZE] = {0};

    content = calloc(sizeof(char) * length + 1, sizeof(char));
    if (content == NULL) {
        perror("Failed to allocate buffer");
        return -1;
    }
    memset(content, '\0', length);
    ptr = content;

    fread(content, 1, length, f);
    content[length] = '\0';


    get_props_name(props_name, filename);
    snprintf(main_function_name, sizeof(main_function_name), "render_%s", filename);
    snprintf(file_id, sizeof(file_id), "%s", filename);


    char *props_start = strstr(content, "({")+2;
    if(props_start != NULL) {
        char *props_end = strstr(props_start, "})");
        if(props_end == NULL){
            perror("Could not find }) closing props struct.");
            free(content);
            return -1;
        }
        strncat(props_struct, props_start, props_end-props_start);
        ptr = props_end+2;
    }


    char *first_html = strstr(ptr, "\n<");
    if(first_html == NULL){
        perror("No HTML found, aborting.");
        free(content);
        return -1;
    }
    strncat(prepend, ptr, first_html-ptr);
    ptr = first_html+1;


    char *code_start, *code_end;

    while((code_start = strstr(ptr, "{{")) && (code_end = strstr(ptr, "}}"))){
        char text[BUFFER_SIZE/2] = {0};
        strncpy(text, ptr, code_start - ptr);
        process_text(text);
        snprintf(function_code + strlen(function_code), BUFFER_SIZE, "\tstrcat(output, \"%s\");\n", text);
        response_length += strlen(text);

        char code[BUFFER_SIZE] = {0};
        strncpy(code, code_start+2, code_end-code_start-2);
        process_code(code);
        strcat(function_code, code);
        response_length += 10+strlen(code);

        ptr = code_end + 2;
    }

    if(ptr != NULL && *ptr != '\0') {
        char remaining[BUFFER_SIZE];
        strcpy(remaining, ptr);
        process_text(remaining);
    }


    char *c_output = malloc(BUFFER_SIZE);
    char *h_output = malloc(BUFFER_SIZE);

    if(c_output == NULL || h_output == NULL) {
        perror("Error allocating memory");
        free(content);
        return -1;
    }

    strcpy(c_output, ctemp);
    strcpy(h_output, htemp);


    char response_length_buffer[128] = {0};
    snprintf(response_length_buffer, 128, "%ld", response_length);


    Placeholder c_placeholders[6] = {
        {"%%CODE%%", function_code},
        {"%%FUNC_NAME%%", main_function_name},
        {"%%PROPS_NAME%%", props_name},
        {"%%PREPEND%%", prepend},
        {"%%NAME%%", file_id},
        {"%%RESPONSE_SIZE%%", response_length_buffer}
    };

    for(int i = 0; i < 6; i++){
        int r = replace_placeholder(&c_output, c_placeholders[i].placeholder, c_placeholders[i].replacement);
        if(r < 0){
            perror("Replace error");
            free(c_output);
            free(h_output);
            free(content);
            return -1;
        }
    }

    Placeholder h_placeholders[4] = {
        {"%%FILE_ID%%", file_id},
        {"%%PROPS%%", props_struct},
        {"%%STRUCT_NAME%%", props_name},
        {"%%FUNC_NAME%%", main_function_name},
    };

    for(int i = 0; i < 4; i++){
        int r = replace_placeholder(&h_output, h_placeholders[i].placeholder, h_placeholders[i].replacement);
        if(r < 0){
            perror("Replace error");
            free(c_output);
            free(h_output);
            free(content);
            return -1;
        }
    }



    char c_file[MAX_FILE_NAME] = {0};
    snprintf(c_file, sizeof(c_file), "%s%s.c", SAVE_PATH, filename);
    if(save_file(c_file, c_output) != 0){
        free(c_output);
        free(h_output);
        free(content);
        return -1;
    }

    char h_file[MAX_FILE_NAME] = {0};
    snprintf(h_file, sizeof(h_file), "%s%s.h", SAVE_PATH, filename);
    if(save_file(h_file, h_output) != 0){
        free(c_output);
        free(h_output);
        free(content);
        return -1;
    }

    free(c_output);
    free(h_output);
    free(content);
    return 0;
}



int main(){
    char *ctemplate = get_template(C);
    char *htemplate = get_template(H);

    if(ctemplate == NULL || htemplate == NULL){
        return 1;
    }

    struct dirent *entry;
    DIR *dir = opendir(PATH_TO_CX_FILES);

    if (dir == NULL) {
        perror("Unable to open directory");
        return EXIT_FAILURE;
    }

    printf("Starting CXC\n");

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char path[1024] = {0};
            char filename[MAX_NAME] = {0};
            snprintf(path, 1024, "%s/%s", PATH_TO_CX_FILES, entry->d_name);
            strncpy(filename, entry->d_name, strchr(entry->d_name, '.')-entry->d_name);

            if(has_cx_extension(path)){
                FILE *file = fopen(path, "r");
                if(file == NULL){
                    perror("Failed to fopen");
                    continue;
                }
                long length = get_file_length(file);
                if(length == 1110){
                    perror("Length <= 0");
                    fclose(file);
                    continue;
                }

                int res = generate(file, filename, length, ctemplate, htemplate);

                fclose(file);
                if(res < 0){
                    closedir(dir);
                    free(ctemplate);
                    free(htemplate);
                    return EXIT_FAILURE;
                } else{
                    printf("Generated %s.c and %s.h in src/cxc.\n", filename, filename);
                }
             }
        }
    }
    closedir(dir);
    free(ctemplate);
    free(htemplate);

    return EXIT_SUCCESS;
}
