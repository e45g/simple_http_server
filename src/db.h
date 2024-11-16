#ifndef DB_H
#define DB_H

typedef int (*db_callback)(void*,int,char**,char**);

#define MAX_ERROR_LEN 256
#define MAX_QUERY_LEN 4096

typedef struct {
    char ***rows;
    char **col_names;
    int num_rows;
    int num_cols;
} DBResult;

int db_init(const char *db_path);
void db_close(void);

int db_exec(const char *query, db_callback callback, void *callback_data, char **error_msg);
DBResult *db_query(char *query, const char **params, int params_count);
void free_result(DBResult *result);

int db_execute(const char *sql, const char **params, int param_count);
int db_exists(const char *id);

#endif
