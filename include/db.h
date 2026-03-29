#ifndef DB_H
#define DB_H

#include <sqlite3.h>

#define DB_NAME "fileshare.db"

sqlite3* init_db();
void close_db(sqlite3 *db);
int create_tables(sqlite3 *db);
int insert_user(sqlite3 *db, const char *username, const char *password);
int verify_user(sqlite3 *db, const char *username, const char *password);
int insert_file(sqlite3 *db, const char *filename, const char *path, int user_id);
int get_file_path(sqlite3 *db, int file_id, char *path, size_t path_size);
int create_share_link(sqlite3 *db, int file_id, char *link, size_t link_size);
int get_shared_file(sqlite3 *db, const char *link, char *path, size_t path_size);
char* get_user_files(sqlite3 *db, int user_id);
int get_filename(sqlite3 *db, int file_id, char *filename, size_t filename_size);

#endif