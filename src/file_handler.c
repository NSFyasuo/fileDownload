#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "server.h"
#include "file_handler.h"
#include "db.h"

int handle_file_upload(int client_fd, const char *request, size_t request_len, sqlite3 *db, int user_id) {
    char filename[256];
    char *file_data;
    size_t file_size;

    if (!parse_multipart(request, request_len, filename, &file_data, &file_size)) {
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

    char filename[256];
    if (!get_filename(db, file_id, filename, sizeof(filename))) {
        strcpy(filename, "downloaded_file");
    }

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        const char *error = "HTTP/1.1 404 Not Found\r\n\r\nFile not found";
        write(client_fd, error, strlen(error));
        return 0;
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
    return 1;
}

int parse_multipart(const char *request, size_t request_len, char *filename, char **file_data, size_t *file_size) {
    // Find boundary in headers
    const char *boundary_key = "boundary=";
    const char *boundary_start = memmem(request, request_len, boundary_key, strlen(boundary_key));
    if (!boundary_start) return 0;
    boundary_start += strlen(boundary_key);

    // boundary ends at CRLF or end
    const char *boundary_end = memchr(boundary_start, '\r', request_len - (boundary_start - request));
    if (!boundary_end) boundary_end = request + request_len;
    size_t boundary_len = boundary_end - boundary_start;
    if (boundary_len == 0 || boundary_len >= 250) return 0;

    char boundary[256];
    memcpy(boundary, boundary_start, boundary_len);
    boundary[boundary_len] = '\0';

    char boundary_delim[300];
    int n = snprintf(boundary_delim, sizeof(boundary_delim), "--%s", boundary);
    if (n <= 0) return 0;

    // Find headers/body separator
    const char *data_start = memmem(request, request_len, "\r\n\r\n", 4);
    if (!data_start) return 0;
    data_start += 4;

    // Find filename in part header (still in header region before file data)
    const char *filename_key = "filename=\"";
    const char *fn = memmem(request, request_len, filename_key, strlen(filename_key));
    if (!fn) return 0;
    fn += strlen(filename_key);
    const char *fn_end = memchr(fn, '"', request_len - (fn - request));
    if (!fn_end) return 0;
    size_t fn_len = fn_end - fn;
    if (fn_len == 0 || fn_len >= 255) return 0;
    memcpy(filename, fn, fn_len);
    filename[fn_len] = '\0';

    // Find boundary that marks end of file content
    char end_boundary[300];
    n = snprintf(end_boundary, sizeof(end_boundary), "\r\n--%s", boundary);
    if (n <= 0) return 0;

    const char *file_end = memmem(data_start, request_len - (data_start - request), end_boundary, strlen(end_boundary));
    if (!file_end) return 0;

    *file_size = file_end - data_start;
    *file_data = malloc(*file_size);
    if (!*file_data) return 0;
    memcpy(*file_data, data_start, *file_size);

    return 1;
}