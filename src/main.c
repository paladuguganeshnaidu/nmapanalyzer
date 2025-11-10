#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "db.h"
#include "server.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc >= 3) {
        const char *target = argv[1];
        int level = atoi(argv[2]);
        struct ScanResult result;
        memset(&result, 0, sizeof(result));

    db_init("data/scans.db");

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "Failed to initialize Winsock.\n");
        return 1;
    }
#endif

    run_nmap_scan(target, level, &result);
        print_result(&result);

        // Save as best-effort
        if (db_is_ready()) {
            if (db_save_scan(target, result.target_ip, result.timestamp, level, "") != 0) {
                fprintf(stderr, "Failed to save scan to DB for %s\n", target);
            }
        }

#ifdef _WIN32
    WSACleanup();
#endif
    db_close();
        return 0;
    }

    // No args: start server
    // If you prefer not to start the HTTP server here, pass args to run a CLI scan.
    server_start(8080);
    return 0;
}
