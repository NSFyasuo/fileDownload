#ifndef AUTH_H
#define AUTH_H

#include <sqlite3.h>

#define SESSION_TOKEN_SIZE 32

typedef struct {
    int user_id;
    char token[SESSION_TOKEN_SIZE + 1];
} Session;

int authenticate_user(sqlite3 *db, const char *username, const char *password, Session *session);
int validate_session(sqlite3 *db, const char *token, int *user_id);

#endif