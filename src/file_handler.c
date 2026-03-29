#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "server.h"
#include "file_handler.h"
#include "db.h"

int handle_file_upload(int client_fd, const char *request, sqlite3 *db, int user_id) {
    char filename[256];
    char *file_data;
    size_t file_size;

    if (!parse_multipart(request, filename, &file_data, &file_size)) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid upload";
        write(client_fd, error, strlen(error));
        return 0;
    }

    char filepath[512];
    sprintf(filepath, "%s%s", UPLOAD_DIR, filename);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        const char *error = "HTTP/1.1 500 Internal Server Error\r\n\r\nFailed to save file";
        write(client_fd, error, strlen(error));
        free(file_data);
        return 0;
    }

    write(fd, file_data, file_size);
    close(fd);
    free(file_data);

    insert_file(db, filename, filepath, user_id);

    const char *response = "HTTP/1.1 200 OK\r\n\r\nFile uploaded successfully";
    write(client_fd, response, strlen(response));
    return 1;
}

int handle_file_download(int client_fd, const char *path, sqlite3 *db) {
    // Parse file_id from path, e.g., /download?id=1
    char *query = strchr(path, '?');
    if (!query) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nMissing file id";
        write(client_fd, error, strlen(error));
        return 0;
    }
    char *id_str = strstr(query, "id=");
    if (!id_str) {
        const char *error = "HTTP/1.1 400 Bad Request\r\n\r\nMissing file id";
        write(client_fd, error, strlen(error));
        return 0;
    }
    int file_id = atoi(id_str + 3);

    char filepath[512];
    if (!get_file_path(db, file_id, filepath, sizeof(filepath))) {
        const char *error = "HTTP/1.1 404 Not Found\r\n\r\nFile not found";
        write(client_fd, error, strlen(error));
        return 0;
    }

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        const char *error = "HTTP/1.1 404 Not Found\r\n\r\nFile not found";
        write(client_fd, error, strlen(error));
        return 0;
    }

    const char *header = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n\r\n";
    write(client_fd, header, strlen(header));

    char buffer[BUFFER_SIZE];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        write(client_fd, buffer, bytes);
    }

    fclose(file);
    return 1;
}

int parse_multipart(const char *request, char *filename, char **file_data, size_t *file_size) {
    // Find boundary
    const char *boundary_start = strstr(request, "boundary=");
    if (!boundary_start) return 0;
    boundary_start += 9;
    char boundary[256];
    sscanf(boundary_start, "%255[^;\r\n]", boundary);

    char boundary_delim[300];
    sprintf(boundary_delim, "--%s", boundary);

    // Find start of data
    const char *data_start = strstr(request, "\r\n\r\n");
    if (!data_start) return 0;
    data_start += 4;

    // Find filename
    const char *fn = strstr(data_start, "filename=\"");
    if (!fn) return 0;
    fn += 10;
    const char *fn_end = strchr(fn, '"');
    if (!fn_end) return 0;
    size_t fn_len = fn_end - fn;
    strncpy(filename, fn, fn_len);
    filename[fn_len] = '\0';

    // Find file data start
    const char *file_start = strstr(fn_end, "\r\n\r\n");
    if (!file_start) return 0;
    file_start += 4;

    // Find end of file data
    char end_boundary[300];
    sprintf(end_boundary, "\r\n--%s--", boundary);
    const char *file_end = strstr(file_start, end_boundary);
    if (!file_end) return 0;

    *file_size = file_end - file_start;
    *file_data = malloc(*file_size);
    memcpy(*file_data, file_start, *file_size);

    return 1;
}