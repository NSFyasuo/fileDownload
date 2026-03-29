#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "server.h"
#include "db.h"
#include "auth.h"
#include "file_handler.h"

sqlite3 *db;

void init_server(Server *server) {
    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(1);
    }

    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(PORT);

    if (bind(server->socket_fd, (struct sockaddr *)&server->address, sizeof(server->address)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server->socket_fd, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }

    db = init_db();
    create_tables(db);
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    handle_request(client_fd);
    close(client_fd);
    return NULL;
}

void start_server(Server *server) {
    printf("Server listening on port %d\n", PORT);
    while (1) {
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server->socket_fd, NULL, NULL);
        if (*client_fd < 0) {
            perror("Accept failed");
            free(client_fd);
            continue;
        }

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_fd);
        pthread_detach(thread);
    }
}

void handle_request(int client_fd) {
    char *buffer = malloc(MAX_REQUEST_SIZE);
    if (!buffer) {
        const char *error = "HTTP/1.1 500 Internal Server Error\r\n\r\nMemory allocation failed";
        write(client_fd, error, strlen(error));
        return;
    }
    ssize_t bytes_read = read(client_fd, buffer, MAX_REQUEST_SIZE - 1);
    if (bytes_read < 0) {
        perror("Read failed");
        free(buffer);
        return;
    }
    buffer[bytes_read] = '\0';

    // Parse request line
    char method[16], path[256], version[16];
    sscanf(buffer, "%15s %255s %15s", method, path, version);

    // Extract token from headers
    char *auth_header = strstr(buffer, "Authorization: Bearer ");
    int user_id = 0;
    if (auth_header) {
        char token[SESSION_TOKEN_SIZE + 1];
        sscanf(auth_header + 22, "%s", token);
        validate_session(db, token, &user_id);
    }

    if (strcmp(method, "POST") == 0 && strstr(path, "/upload")) {
        if (user_id == 0) {
            const char *error = "HTTP/1.1 401 Unauthorized\r\n\r\nPlease login";
            write(client_fd, error, strlen(error));
            free(buffer);
            return;
        }
        handle_file_upload(client_fd, buffer, bytes_read, db, user_id);
    } else if (strcmp(method, "GET") == 0 && strstr(path, "/download")) {
        handle_file_download(client_fd, path, db);
    } else if (strcmp(method, "POST") == 0 && strstr(path, "/login")) {
        handle_login(client_fd, buffer, db);
    } else if (strcmp(method, "POST") == 0 && strstr(path, "/register")) {
        handle_register(client_fd, buffer, db);
    } else if (strcmp(method, "POST") == 0 && strstr(path, "/share")) {
        if (user_id == 0) {
            const char *error = "HTTP/1.1 401 Unauthorized\r\n\r\nPlease login";
            write(client_fd, error, strlen(error));
            free(buffer);
            return;
        }
        handle_share(client_fd, path, db, user_id);
    } else if (strcmp(method, "GET") == 0 && strstr(path, "/shared")) {
        handle_shared_download(client_fd, path, db);
    } else if (strcmp(method, "GET") == 0 && strstr(path, "/files")) {
        if (user_id == 0) {
            const char *error = "HTTP/1.1 401 Unauthorized\r\n\r\nPlease login";
            write(client_fd, error, strlen(error));
            free(buffer);
            return;
        }
        handle_files(client_fd, db, user_id);
    } else {
        // Serve static files or default response
        serve_static(client_fd, path);
    }
    free(buffer);
}

void handle_login(int client_fd, const char *request, sqlite3 *db) {
    // Parse username and password from request body
    char *body = strstr(request, "\r\n\r\n");
    if (!body) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid request";
        write(client_fd, error, strlen(error));
        return;
    }
    char username[256], password[256];
    sscanf(body + 4, "username=%255[^&]&password=%255s", username, password);

    Session session;
    if (authenticate_user(db, username, password, &session)) {
        char response[512];
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"token\":\"%s\"}", session.token);
        write(client_fd, response, strlen(response));
    } else {
        const char *error = "HTTP/1.1 401 Unauthorized\r\n\r\nInvalid credentials";
        write(client_fd, error, strlen(error));
    }
}

void handle_register(int client_fd, const char *request, sqlite3 *db) {
    char *body = strstr(request, "\r\n\r\n");
    if (!body) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid request";
        write(client_fd, error, strlen(error));
        return;
    }
    char username[256], password[256];
    sscanf(body + 4, "username=%255[^&]&password=%255s", username, password);

    if (insert_user(db, username, password)) {
        const char *response = "HTTP/1.1 201 Created\r\n\r\nUser registered";
        write(client_fd, response, strlen(response));
    } else {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nRegistration failed";
        write(client_fd, error, strlen(error));
    }
}

void handle_share(int client_fd, const char *path, sqlite3 *db, int user_id) {
    char *query = strchr(path, '?');
    if (!query) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nMissing file id";
        write(client_fd, error, strlen(error));
        return;
    }
    char *id_str = strstr(query, "id=");
    if (!id_str) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nMissing file id";
        write(client_fd, error, strlen(error));
        return;
    }
    int file_id = atoi(id_str + 3);

    char link[256];
    if (create_share_link(db, file_id, link, sizeof(link))) {
        char response[512];
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"link\":\"%s\"}", link);
        write(client_fd, response, strlen(response));
    } else {
        const char *error = "HTTP/1.1 500 Internal Server Error\r\n\r\nFailed to create share link";
        write(client_fd, error, strlen(error));
    }
}

void handle_shared_download(int client_fd, const char *path, sqlite3 *db) {
    char *query = strchr(path, '?');
    if (!query) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nMissing link";
        write(client_fd, error, strlen(error));
        return;
    }
    char *link_str = strstr(query, "link=");
    if (!link_str) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nMissing link";
        write(client_fd, error, strlen(error));
        return;
    }
    char link[256];
    sscanf(link_str + 5, "%255s", link);

    char filepath[512];
    if (!get_shared_file(db, link, filepath, sizeof(filepath))) {
        const char *error = "HTTP/1.1 404 Not Found\r\n\r\nShared file not found";
        write(client_fd, error, strlen(error));
        return;
    }

    // Get filename from path (simple way, or query db)
    char *filename = strrchr(filepath, '/');
    if (filename) filename++;
    else filename = "shared_file";

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        const char *error = "HTTP/1.1 404 Not Found\r\n\r\nFile not found";
        write(client_fd, error, strlen(error));
        return;
    }

    char header[1024];
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Disposition: attachment; filename=\"%s\"\r\n\r\n", filename);
    write(client_fd, header, strlen(header));

    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        write(client_fd, buffer, bytes);
    }

    fclose(file);
}

void handle_files(int client_fd, sqlite3 *db, int user_id) {
    char *json = get_user_files(db, user_id);
    if (json) {
        char response[2048];
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n%s", json);
        write(client_fd, response, strlen(response));
        free(json);
    } else {
        const char *error = "HTTP/1.1 500 Internal Server Error\r\n\r\nFailed to get files";
        write(client_fd, error, strlen(error));
    }
}

void serve_static(int client_fd, const char *path) {
    char file_path[512];
    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        strcpy(file_path, "web/index.html");
    } else if (strcmp(path, "/register.html") == 0) {
        strcpy(file_path, "web/register.html");
    } else if (strncmp(path, "/", 1) == 0) {
        // map /xyz to web/xyz
        snprintf(file_path, sizeof(file_path), "web%s", path);
    } else {
        strcpy(file_path, "web/index.html");
    }

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        const char *error = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
        write(client_fd, error, strlen(error));
        return;
    }

    const char *content_type = "application/octet-stream";
    if (strstr(file_path, ".html")) content_type = "text/html";
    else if (strstr(file_path, ".css")) content_type = "text/css";
    else if (strstr(file_path, ".js")) content_type = "application/javascript";
    else if (strstr(file_path, ".json")) content_type = "application/json";
    else if (strstr(file_path, ".png")) content_type = "image/png";
    else if (strstr(file_path, ".jpg") || strstr(file_path, ".jpeg")) content_type = "image/jpeg";

    char header[256];
    sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
    write(client_fd, header, strlen(header));

    char buffer[1024];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        write(client_fd, buffer, bytes);
    }

    fclose(file);
}

int main() {
    Server server;
    init_server(&server);
    start_server(&server);
    close_db(db);
    return 0;
}