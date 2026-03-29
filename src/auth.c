#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "auth.h"
#include "db.h"

int authenticate_user(sqlite3 *db, const char *username, const char *password, Session *session) {
    int user_id = verify_user(db, username, password);
    if (user_id > 0) {
        // Generate simple token (in real app, use proper crypto)
        srand(time(NULL));
        sprintf(session->token, "%d_%ld", user_id, rand());
        session->user_id = user_id;
        return 1;
    }
    return 0;
}

int validate_session(sqlite3 *db, const char *token, int *user_id) {
    // Simple validation, parse token
    char temp_token[256];
    strncpy(temp_token, token, sizeof(temp_token) - 1);
    temp_token[sizeof(temp_token) - 1] = '\0';
    char *underscore = strchr(temp_token, '_');
    if (!underscore) return 0;
    *underscore = '\0';
    *user_id = atoi(temp_token);
    // In real app, check against stored sessions
    return *user_id > 0;
}