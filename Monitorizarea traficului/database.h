#include "sqlite3.h"
#include <stdio.h>
#include <string.h>


sqlite3* db;
int db_init = 0;

void close_database() 
{
    sqlite3_close(db);
}

int init_database()
{
    char* err_msg = 0;
    if (db_init==1)
        return 1;  
    int rc = sqlite3_open("conturi.db", &db);
    if (rc!=SQLITE_OK) 
    {
        fprintf(stderr, "Nu poate fi deschisa baza de date: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    char* sql = "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY,is_logged_in INTEGER DEFAULT 0,client_id INTEGER DEFAULT -1,subscribed INTEGER DEFAULT 0,speed INTEGER DEFAULT 0,street TEXT DEFAULT '');";
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    db_init = 1;
    return 1;
}

int add_user(char* nume)
{
    char sql[101];
    char* err_msg = 0;
    snprintf(sql, sizeof(sql), "INSERT INTO users (username) VALUES ('%s');", nume);
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int set_client_id(char* nume, int client_id) 
{
    char sql[101];
    char* err_msg = 0;
    snprintf(sql, sizeof(sql), "UPDATE users SET client_id = %d WHERE username = '%s';", client_id, nume);
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

char* get_username_by_client_id(int client_id) 
{
    char sql[101];
    static char nume[100];
    sqlite3_stmt* stmt;
    
    snprintf(sql, sizeof(sql), "SELECT username FROM users WHERE client_id = %d;", client_id);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return NULL;
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) 
    {
        char* r = sqlite3_column_text(stmt, 0);
        if (r) {
            strncpy(nume, (char*)r, sizeof(nume) - 1);
            nume[sizeof(nume) - 1] = '\0';
            sqlite3_finalize(stmt);
            return nume;
        }
    }
    else
    {
           sqlite3_finalize(stmt);
            return NULL;
    }
}

int check_user_exists(char* nume) 
{
    char sql[101];
    sqlite3_stmt* stmt;
    snprintf(sql, sizeof(sql), "SELECT username FROM users WHERE username = '%s';", nume);
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }
    
    rc = sqlite3_step(stmt);
    if(rc==SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return 1;
    }
    else
    {
        sqlite3_finalize(stmt);
        return 0;
    }
}

int check_user_logged_in(char* nume) 
{
    char sql[101];
    sqlite3_stmt* stmt;
    snprintf(sql, sizeof(sql), "SELECT is_logged_in FROM users WHERE username = '%s';", nume);
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        return 0;
    }
    
    rc = sqlite3_step(stmt);
    int result;
    if(rc==SQLITE_ROW)
        result = sqlite3_column_int(stmt, 0);
    else
        result = 0;
    sqlite3_finalize(stmt);
    return result;
}

int set_user_logged_in(char* nume, int status) 
{
    char sql[101];
    char* err_msg = 0;
    snprintf(sql, sizeof(sql), "UPDATE users SET is_logged_in = %d WHERE username = '%s';", status, nume);
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int subscribe_user(char* nume) {
    char sql[101];
    char* err_msg = 0;
    snprintf(sql, sizeof(sql), "UPDATE users SET subscribed = 1 WHERE username = '%s';", nume);
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) 
    {
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int unsubscribe_user(char* nume) {
    char sql[101];
    char* err_msg = 0;
    snprintf(sql, sizeof(sql), "UPDATE users SET subscribed = 0 WHERE username = '%s';", nume);
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) 
    {
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int is_user_subscribed(char* nume) 
{
    char sql[101];
    sqlite3_stmt* stmt;
    snprintf(sql, sizeof(sql), "SELECT subscribed FROM users WHERE username = '%s';", nume);
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
        return 0;
    
    rc = sqlite3_step(stmt);
    int result;
    if(rc == SQLITE_ROW)
        result = sqlite3_column_int(stmt, 0);
    else
        result = 0;
    sqlite3_finalize(stmt);
    return result;
}

int logout_user(char* nume) 
{
    return set_user_logged_in(nume, 0);
}

int update_user_speed(char* nume, int speed) {
    char sql[101];
    char* err_msg = 0;
    
    snprintf(sql, sizeof(sql), "UPDATE users SET speed = %d WHERE username = '%s';",speed, nume);
    
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to update speed: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int update_user_street(char* nume, const char* strada) {
    char sql[101];
    char* err_msg = 0;
    
    snprintf(sql, sizeof(sql), "UPDATE users SET street = '%s' WHERE username = '%s';",strada, nume);
    
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to update street: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    return 1;
}

int get_user_speed(char* nume) {
    sqlite3_stmt* stmt;
    int speed = 0;
    
    char* sql = "SELECT speed FROM users WHERE username = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, nume, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            speed = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return speed;
}

char* get_user_street(char* nume) {
    sqlite3_stmt* stmt;
    static char strada[100];
    
    char* sql = "SELECT street FROM users WHERE username = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, nume, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            char* r = (char*)sqlite3_column_text(stmt, 0);
            if (r) {
                strncpy(strada, r, sizeof(strada) - 1);
                strada[sizeof(strada) - 1] = '\0';
            }
        }
        sqlite3_finalize(stmt);
    }
    return strada;
}