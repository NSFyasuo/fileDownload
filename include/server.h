#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <sqlite3.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_REQUEST_SIZE 10485760  // 10MB for larger files

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
} Server;

void init_server(Server *server);
void start_server(Server *server);
void handle_request(int client_fd);
void handle_login(int client_fd, const char *request, sqlite3 *db);
void handle_register(int client_fd, const char *request, sqlite3 *db);
void handle_share(int client_fd, const char *path, sqlite3 *db, int user_id);
void handle_shared_download(int client_fd, const char *path, sqlite3 *db);
void handle_files(int client_fd, sqlite3 *db, int user_id);
void handle_delete_file(int client_fd, const char *path, sqlite3 *db, int user_id);
void serve_static(int client_fd, const char *path);
int handle_file_upload(int client_fd, const char *request, size_t request_len, sqlite3 *db, int user_id);

#endif