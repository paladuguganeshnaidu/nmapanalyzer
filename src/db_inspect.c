/* Simple DB inspector: opens the configured SQLite DB via the project's DB API
 * and prints the JSON returned by db_get_history_json().
 * This is safe and read-only.
 */
#include <stdio.h>
#include <stdlib.h>
#include "db.h"

int main(void) {
    const char *db_path = "data/scans.db";
    printf("Opening DB: %s\n", db_path);
    if (db_init(db_path) != 0) {
        fprintf(stderr, "db_init failed\n");
        return 1;
    }

    if (!db_is_ready()) {
        fprintf(stderr, "DB is not ready\n");
        db_close();
        return 2;
    }

    char *json = db_get_history_json();
    if (!json) {
        printf("No history rows or query failed (NULL returned)\n");
    } else {
        printf("History JSON:\n%s\n", json);
        free(json);
    }

    db_close();
    return 0;
}
