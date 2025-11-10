#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include "db.h"

static sqlite3 *g_db = NULL;

int db_init(const char *db_path) {
    if (sqlite3_open(db_path, &g_db) != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(g_db));
        return -1;
    }

    const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS scans ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "target TEXT, "
        "ip TEXT, "
        "timestamp TEXT, "
        "level INTEGER, "
        "result TEXT);";

    char *errmsg = NULL;
    if (sqlite3_exec(g_db, create_table_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "DB init failed: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(g_db);
        g_db = NULL;
        return -1;
    }
    return 0;
}

void db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

int db_is_ready(void) {
    return (g_db != NULL);
}

int db_save_scan(const char *target, const char *ip, const char *timestamp,
                 int level, const char *result_json) {
    if (!g_db) return -1;

    const char *insert_sql =
        "INSERT INTO scans (target, ip, timestamp, level, result) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(g_db, insert_sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, target, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ip, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, timestamp, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, level);
    sqlite3_bind_text(stmt, 5, result_json, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

char *db_get_history_json(void) {
    if (!g_db) return NULL;

    const char *query_sql =
        "SELECT target, ip, timestamp, level, result FROM scans ORDER BY id DESC;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(g_db, query_sql, -1, &stmt, NULL) != SQLITE_OK)
        return NULL;

    char *json = malloc(8192);
    if (!json) return NULL;
    strcpy(json, "[");

    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) strcat(json, ",");
        first = 0;

        char row[1024];
        snprintf(row, sizeof(row),
                 "{\"target\":\"%s\",\"ip\":\"%s\",\"timestamp\":\"%s\",\"level\":%d}",
                 sqlite3_column_text(stmt, 0),
                 sqlite3_column_text(stmt, 1),
                 sqlite3_column_text(stmt, 2),
                 sqlite3_column_int(stmt, 3));
        strcat(json, row);
    }
    strcat(json, "]");
    sqlite3_finalize(stmt);
    return json;
}
