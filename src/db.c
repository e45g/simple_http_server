#include <pthread.h>
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "db.h"

static sqlite3 *db = NULL;

static void handle_db_error(const char* operation, const char* error_msg) {
    char error_buf[MAX_ERROR_LEN];
    snprintf(error_buf, MAX_ERROR_LEN, "Database error during %s: %s",
             operation, error_msg ? error_msg : "unknown error");
    LOG("DB ERROR: %s", error_buf);
}

int db_init(const char *db_path){
    if(db){
        LOG("DB already initialized");
        return 0;
    }
    int rc = sqlite3_open(db_path, &db);
    if(rc != SQLITE_OK){
        handle_db_error("initialization", sqlite3_errmsg(db));
        sqlite3_close(db);
        db = NULL;
        return -1;
    }

    LOG("Database initialized successfully: %s", db_path);
    return 0;
}

void db_close(void){
    if(db){
        sqlite3_close(db);
        db = NULL;
    }
}

int db_exec(const char *query, db_callback callback, void *callback_data, char **error_msg){
    if(!db){
        handle_db_error("query execution", "Database not initialized");
        return -1;
    }

    int rc = sqlite3_exec(db, query, callback, callback_data, error_msg);
    if (rc != SQLITE_OK && error_msg && *error_msg) {
        handle_db_error("execution", *error_msg);
        sqlite3_free(*error_msg);
    }
    return rc;
}

int db_execute(const char *sql, const char **params, int param_count) {
    sqlite3_stmt *stmt;

    if(!db){
        handle_db_error("sql query execution", "Database not initialized");
        return -1;
    }

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        handle_db_error("execute_sql_with_placeholders", sqlite3_errmsg(db));
        return -1;
    }

    for (int i = 0; i < param_count; i++) {
        if (sqlite3_bind_text(stmt, i + 1, params[i], -1, SQLITE_STATIC) != SQLITE_OK) {
            handle_db_error("binding parameters", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return -1;
        }
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        handle_db_error("execution failed", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

void free_result(DBResult *result){
    if(!result) return;

    if(result->col_names) {
        for(int i = 0; i < result->num_cols; i++){
            free(result->col_names[i]);
        }
        free(result->col_names);
    }

    if(result->rows){
        for(int i = 0; i < result->num_rows; i++){
            for (int j = 0; j < result->num_cols; j++) {
                free(result->rows[i][j]);
            }
            free(result->rows[i]);
        }
        free(result->rows);
    }

    free(result);
}

DBResult *create_result(void){
    DBResult *result = malloc(sizeof(DBResult));

    if (!result) {
        handle_db_error("create result", "Memory allocation failed");
        return NULL;
    }

    result->rows = NULL;
    result->num_rows = 0;
    result->num_cols = 0;
    result->col_names = NULL;

    return result;
}

DBResult *db_query(char *query, const char **params, int params_count){
    if(!db){
        handle_db_error("DB query", "Database not initialized");
        return NULL;
    }

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if(rc != SQLITE_OK){
        handle_db_error("stmt sqlite3_prepare_v2", sqlite3_errmsg(db));
        return NULL;
    }

    for(int i=0; i < params_count; i++){
        if(sqlite3_bind_text(stmt, i+1, params[i], -1, SQLITE_STATIC) != SQLITE_OK){
            handle_db_error("binding parameters", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return NULL;
        }
    }

    DBResult *result = create_result();
    if(!result){
        sqlite3_finalize(stmt);
        return NULL;
    }

    result->num_cols = sqlite3_column_count(stmt);
    result->col_names = malloc(sizeof(char*) * result->num_cols);
    if(!result->col_names){
        sqlite3_finalize(stmt);
        free_result(result);
        return NULL;
    }

    for(int i = 0; i < result->num_cols; i++){
        const char *col_name = sqlite3_column_name(stmt, i);
        result->col_names[i] = strdup(col_name);
        if(!result->col_names[i]){
            sqlite3_finalize(stmt);
            free_result(result);
            return NULL;
        }
    }

    ssize_t max_rows = 10;
    result->rows = malloc(sizeof(char**) * max_rows);
    if(!result->rows){
        sqlite3_finalize(stmt);
        free_result(result);
        return NULL;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW){
        if(result->num_rows >= max_rows){
            max_rows *= 2;
            char ***new_rows = realloc(result->rows, max_rows * sizeof(char**));
            if(!new_rows){
                sqlite3_finalize(stmt);
                free_result(result);
                return NULL;
            }
            result->rows = new_rows;
        }

        result->rows[result->num_rows] = malloc(sizeof(char*) * result->num_cols);
        if (!result->rows[result->num_rows]) {
            sqlite3_finalize(stmt);
            free_result(result);
            return NULL;
        }


        for(int i = 0; i < result->num_cols; i++){
            const unsigned char *value = sqlite3_column_text(stmt, i);
            result->rows[result->num_rows][i] = value ? strdup((const char*) value) : strdup("");
            if (!result->rows[result->num_rows][i]) {
                sqlite3_finalize(stmt);
                free_result(result);
                return NULL;
            }
        }

        result->num_rows++;
    }

    sqlite3_finalize(stmt);
    return result;
}


int db_exists(const char *id){
    if(!db){
        handle_db_error("DB query", "Database not initialized");
        return -1;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT ID FROM games WHERE ID = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        handle_db_error("stmt sqlite3_prepare_v2", sqlite3_errmsg(db));
        return -1;

    }

    sqlite3_bind_text(stmt, 1, id, -1, SQLITE_STATIC);
    int exists = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = 1;
    }

    sqlite3_finalize(stmt);
    return exists;
}
