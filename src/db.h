#ifndef DB_H
#define DB_H

typedef int (*db_callback)(void*,int,char**,char**);

#define MAX_ERROR_LEN 256
#define MAX_QUERY_LEN 4096

typedef struct {
    char ***rows;        // Array of rows
    char **col_names;    // Array of column names
    int num_rows;        // Number of rows
    int num_cols;        // Number of columns
} DBResult;

int db_init(const char *db_path);
void db_close(void);
int db_exec(const char *query, db_callback callback, void *callback_data, char **error_msg);
DBResult *db_query(char *query);
void free_result(DBResult *result);

#endif
