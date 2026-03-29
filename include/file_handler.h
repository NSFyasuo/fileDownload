#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <sqlite3.h>

#define UPLOAD_DIR "uploads/"

int handle_file_upload(int client_fd, const char *request, sqlite3 *db, int user_id);
int handle_file_download(int client_fd, const char *path, sqlite3 *db);
int parse_multipart(const char *request, char *filename, char **file_data, size_t *file_size);

#endif