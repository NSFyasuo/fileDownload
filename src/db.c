#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "db.h"

sqlite3* init_db() {
    sqlite3 *db;
    int rc = sqlite3_open(DB_NAME, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

void close_db(sqlite3 *db) {
    sqlite3_close(db);
}

int create_tables(sqlite3 *db) {
    const char *sql_users = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, username TEXT UNIQUE, password TEXT);";
    const char *sql_files = "CREATE TABLE IF NOT EXISTS files (id INTEGER PRIMARY KEY, filename TEXT, path TEXT, user_id INTEGER, FOREIGN KEY(user_id) REFERENCES users(id));";
    const char *sql_shares = "CREATE TABLE IF NOT EXISTS shares (id INTEGER PRIMARY KEY, file_id INTEGER, link TEXT UNIQUE, FOREIGN KEY(file_id) REFERENCES files(id));";

    char *err_msg = 0;
    int rc = sqlite3_exec(db, sql_users, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    rc = sqlite3_exec(db, sql_files, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    rc = sqlite3_exec(db, sql_shares, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

int insert_user(sqlite3 *db, const char *username, const char *password) {
    const char *sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

int verify_user(sqlite3 *db, const char *username, const char *password) {
    const char *sql = "SELECT id FROM users WHERE username = ? AND password = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    int user_id = 0;
    if (rc == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    return user_id;
}

int insert_file(sqlite3 *db, const char *filename, const char *path, int user_id) {
    const char *sql = "INSERT INTO files (filename, path, user_id) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, user_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

int get_file_path(sqlite3 *db, int file_id, char *path, size_t path_size) {
    const char *sql = "SELECT path FROM files WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, file_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char *p = (const char *)sqlite3_column_text(stmt, 0);
        strncpy(path, p, path_size - 1);
        path[path_size - 1] = '\0';
    }
    sqlite3_finalize(stmt);

    return rc == SQLITE_ROW;
}

int create_share_link(sqlite3 *db, int file_id, char *link, size_t link_size) {
    // Generate random link
    srand(time(NULL));
    sprintf(link, "%d_%ld", file_id, rand());

    const char *sql = "INSERT INTO shares (file_id, link) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, file_id);
    sqlite3_bind_text(stmt, 2, link, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

int get_shared_file(sqlite3 *db, const char *link, char *path, size_t path_size) {
    const char *sql = "SELECT f.path FROM files f JOIN shares s ON f.id = s.file_id WHERE s.link = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, link, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char *p = (const char *)sqlite3_column_text(stmt, 0);
        strncpy(path, p, path_size - 1);
        path[path_size - 1] = '\0';
    }
    sqlite3_finalize(stmt);

    return rc == SQLITE_ROW;
}

char* get_user_files(sqlite3 *db, int user_id) {
    const char *sql = "SELECT id, filename FROM files WHERE user_id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    char *json = malloc(1024);
    strcpy(json, "[");

    int first = 1;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (!first) strcat(json, ",");
        char item[256];
        sprintf(item, "{\"id\":%d,\"filename\":\"%s\"}", sqlite3_column_int(stmt, 0), sqlite3_column_text(stmt, 1));
        strcat(json, item);
        first = 0;
    }
    strcat(json, "]");

    sqlite3_finalize(stmt);
    return json;
}

int get_filename(sqlite3 *db, int file_id, char *filename, size_t filename_size) {
    const char *sql = "SELECT filename FROM files WHERE id = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_int(stmt, 1, file_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char *fn = (const char *)sqlite3_column_text(stmt, 0);
        strncpy(filename, fn, filename_size - 1);
        filename[filename_size - 1] = '\0';
    }
    sqlite3_finalize(stmt);

    return rc == SQLITE_ROW;
}