#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <sys/stat.h>
#include "db.h"

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    // Ensure data directory exists
    _mkdir("data");

    printf("Initializing database at data/scans.db...\n");
    int rc = db_init("data/scans.db");
    if (rc != 0) {
        fprintf(stderr, "db_init returned error code %d\n", rc);
        return 1;
    }

    // Close DB and report
    db_close();

    // Report file info
    FILE *f = fopen("data/scans.db", "rb");
    if (!f) {
        fprintf(stderr, "Error: could not open data/scans.db after init\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);

    printf("Created data/scans.db (size=%ld bytes)\n", size);
    return 0;
}
