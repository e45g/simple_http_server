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

int replace_placeholder(char **buffer, char *placeholder, char *replacement) {
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

void process_text(char *s){
    char tmp[BUFFER_SIZE] = {0};
    int i = 0;
    int j = 0;

    while(s[i] != '\0' && j < (int)sizeof(tmp) - 1){
        if(s[i] == '\n'){
            tmp[j++] = ' ';
        }
        else if(s[i] == '"'){
            tmp[j++] = '\\';
            tmp[j++] = '"';
        }
        else{
            tmp[j++] = s[i];
        }
        i++;
    }

    tmp[j] = '\0';
    strncpy(s, tmp, strlen(tmp));
}

void process_code(char *s){
    char tmp[BUFFER_SIZE] = {0};
    if(*s == '='){
        strcpy(tmp, "\tstrcat(output, ");
        strncat(tmp, s+1, strlen(s));
        strcat(tmp, ");");
    }
    else{
        strcpy(tmp, "\t");
        strncat(tmp, s, strlen(s));
    }
    strncpy(s, tmp, strlen(tmp));
    strcat(s, "\n");
}

void add_text(char *function_code, char *ptr, char *code_start){
    strcat(function_code, "\tstrcat(output, \"");

    char text[BUFFER_SIZE] = {0};
    strncpy(text, ptr, code_start-ptr);

    process_text(text);

    strncat(function_code, text, strlen(text));
    strcat(function_code, "\");\n");

    response_length += strlen("strcat(output, \"") + strlen(text) + strlen("\");\n");
}

char *get_template(Template t){
    if(t == C){
        FILE *f = fopen(CTEMPLATE_PATH, "r");
        if(f == NULL) {
            perror("Could not open ctemplate.c");
            return NULL;
        }
        long length = get_file_length(f);
        if(length <= 0) {
            perror("Length 0, ctemplate.c");
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
    else if (t == H){
        FILE *f = fopen(HTEMPLATE_PATH, "r");
        if(f == NULL) {
            perror("Could not open htemplate.h");
            return NULL;
        }
        long length = get_file_length(f);
        if(length <= 0) {
            perror("Length 0, htemplate.h");
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
    return NULL;
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


    while(ptr != NULL){
        char *code_start = strstr(ptr, "{{");
        if(code_start == NULL){
            strcat(function_code, "\tstrcat(output, \"");

            char text[BUFFER_SIZE] = {0};
            strcpy(text, ptr);

            process_text(text);

            strcat(function_code, text);
            strcat(function_code, "\");\n");

            break;
        }

        add_text(function_code, ptr, code_start);

        char *code_end = strstr(code_start, "}}")-2;
        if(code_start == NULL){
            perror("Could not find the }}");
            free(content);
            return -1;
        }

        char code[BUFFER_SIZE] = {0};
        strncpy(code, code_start+2, code_end-code_start);
        process_code(code);

        strncat(function_code, code, strlen(code));
        response_length += strlen(code);

        ptr = code_end + 4;
    }
    function_code[strlen(function_code)+1] = '\0';

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


    replace_placeholder(&c_output, "%%CODE%%", function_code);
    replace_placeholder(&c_output, "%%FUNC_NAME%%", main_function_name);
    replace_placeholder(&c_output, "%%PROPS_NAME%%", props_name);
    replace_placeholder(&c_output, "%%PREPEND%%", prepend);
    replace_placeholder(&c_output, "%%NAME%%", file_id);
    replace_placeholder(&c_output, "%%RESPONSE_SIZE%%", response_length_buffer);

    replace_placeholder(&h_output, "%%FILE_ID%%", file_id);
    replace_placeholder(&h_output, "%%PROPS%%", props_struct);
    replace_placeholder(&h_output, "%%STRUCT_NAME%%", props_name);
    replace_placeholder(&h_output, "%%FUNC_NAME%%", main_function_name);


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
