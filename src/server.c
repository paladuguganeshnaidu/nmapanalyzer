#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "scanner.h"
#include "server.h"
#include "db.h"

#ifndef _WIN32
#include <pthread.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 4096

// Function declarations
void handle_request(void* arg);
/* POSIX thread wrapper: pthread requires a function returning void* */
static void *handle_request_thread(void *arg) { handle_request(arg); return NULL; }
void send_response(SOCKET client_socket, const char* content, const char* content_type);
void send_file(SOCKET client_socket, const char* filepath);
void handle_scan_request(SOCKET client_socket, const char* query);
#include <time.h>

// Small helper: escape JSON strings (in-place safe src->dst)
static void json_escape(const char *src, char *dst, size_t dst_len) {
    if (!src || !dst || dst_len == 0) return;
    size_t di = 0;
    const unsigned char *s = (const unsigned char*)src;
    while (*s && di + 1 < dst_len) {
        unsigned char c = *s++;
        if (c == '"') {
            if (di + 2 < dst_len) { dst[di++] = '\\'; dst[di++] = '"'; }
        } else if (c == '\\') {
            if (di + 2 < dst_len) { dst[di++] = '\\'; dst[di++] = '\\'; }
        } else if (c == '\b') {
            if (di + 2 < dst_len) { dst[di++] = '\\'; dst[di++] = 'b'; }
        } else if (c == '\f') {
            if (di + 2 < dst_len) { dst[di++] = '\\'; dst[di++] = 'f'; }
        } else if (c == '\n') {
            if (di + 2 < dst_len) { dst[di++] = '\\'; dst[di++] = 'n'; }
        } else if (c == '\r') {
            if (di + 2 < dst_len) { dst[di++] = '\\'; dst[di++] = 'r'; }
        } else if (c == '\t') {
            if (di + 2 < dst_len) { dst[di++] = '\\'; dst[di++] = 't'; }
        } else if (c < 0x20) {
            /* encode other controls as \u00XX */
            if (di + 6 < dst_len) {
                int n = snprintf(dst + di, dst_len - di, "\\u%04x", c);
                if (n > 0) di += (size_t)n;
            }
        } else {
            dst[di++] = c;
        }
    }
    dst[di] = '\0';
}

int server_start(int port) {
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);

    // Initialize DB (use data/scans.db)
    if (!db_init("data/scans.db")) {
        fprintf(stderr, "Warning: failed to initialize DB, continuing without persistence.\n");
    }

#ifndef _WIN32
    // POSIX: nothing like WSAStartup
#endif

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        return 1;
    }

    // Prepare sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("Bind failed.\n");
        closesocket(server_socket);
        return 1;
    }

    // Listen
    if (listen(server_socket, 3) == -1) {
        printf("Listen failed.\n");
        closesocket(server_socket);
        return 1;
    }

    printf("Server running on http://localhost:%d\n", port);

    // Accept and handle incoming connections
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed.\n");
            continue;
        }

#ifndef _WIN32
        // Create a pthread that calls handle_request
        pthread_t th;
        int rc = pthread_create(&th, NULL, handle_request_thread, (void*)(uintptr_t)client_socket);
        if (rc != 0) {
            printf("Thread creation failed.\n");
            closesocket(client_socket);
        } else {
            pthread_detach(th);
        }
#else
        uintptr_t th = _beginthread(handle_request, 0, (void*)(uintptr_t)client_socket);
        if (th == (uintptr_t)-1) {
            printf("Thread creation failed.\n");
            closesocket(client_socket);
        }
#endif
    }

    closesocket(server_socket);
    return 0;
}

void handle_request(void* arg) {
    SOCKET client_socket = (SOCKET)(uintptr_t)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    // Parse HTTP request (safe)
    char method[16] = {0}, fullpath[256] = {0};
    char path[256] = {0}, query[256] = {0};

    // Extract first line
    char *line_end = strstr(buffer, "\r\n");
    char request_line[512] = {0};
    if (line_end) {
        size_t len = (size_t)(line_end - buffer);
        if (len >= sizeof(request_line)) len = sizeof(request_line) - 1;
        memcpy(request_line, buffer, len);
        request_line[len] = '\0';
    } else {
        strncpy(request_line, buffer, sizeof(request_line) - 1);
    }

    if (sscanf(request_line, "%15s %255s", method, fullpath) < 2) {
        closesocket(client_socket);
        return;
    }

    char *q = strchr(fullpath, '?');
    if (q) {
        size_t p_len = (size_t)(q - fullpath);
        if (p_len >= sizeof(path)) p_len = sizeof(path) - 1;
        memcpy(path, fullpath, p_len);
        path[p_len] = '\0';
        strncpy(query, q + 1, sizeof(query) - 1);
    } else {
        strncpy(path, fullpath, sizeof(path) - 1);
    }
    
    if (strcmp(path, "/") == 0) {
        send_file(client_socket, "www/index.html");
    } else if (strcmp(path, "/style.css") == 0) {
        send_file(client_socket, "www/style.css");
    } else if (strcmp(path, "/script.js") == 0) {
        send_file(client_socket, "www/script.js");
    } else if (strcmp(path, "/scan") == 0) {
        handle_scan_request(client_socket, query);
    } else if (strncmp(path, "/data/", 6) == 0) {
        // Serve files under data/ directory (path begins with /data/). Remove leading '/'
        char filepath[512] = {0};
        if (strlen(path) + 1 < sizeof(filepath)) {
            snprintf(filepath, sizeof(filepath), "%s", path + 1);
            send_file(client_socket, filepath);
        } else {
            char response[] = "HTTP/1.1 400 Bad Request\r\n\r\n";
            send(client_socket, response, strlen(response), 0);
        }
    } else if (strcmp(path, "/history") == 0) {
        // Return JSON array of history
        char *hist = db_get_history_json();
        if (!hist) {
            char response[] = "HTTP/1.1 500 Internal Server Error\r\n\r\n{\"error\":\"no history\"}";
            send(client_socket, response, strlen(response), 0);
        } else {
            send_response(client_socket, hist, "application/json");
            free(hist);
        }
    } else if (strcmp(path, "/status") == 0) {
        // Return DB status and storage path
        char status_json[512];
        int db_ok = db_is_ready();
        snprintf(status_json, sizeof(status_json), "{\"db_ok\": %s, \"storage\": \"data/\"}", db_ok ? "true" : "false");
        send_response(client_socket, status_json, "application/json");
    } else {
        char response[] = "HTTP/1.1 404 Not Found\r\n\r\n<h1>404 Not Found</h1>";
        send(client_socket, response, strlen(response), 0);
    }
    
    closesocket(client_socket);
}

void send_response(SOCKET client_socket, const char* content, const char* content_type) {
    char header[BUFFER_SIZE];
    sprintf(header, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        content_type, strlen(content));
    
    send(client_socket, header, strlen(header), 0);
    send(client_socket, content, strlen(content), 0);
}

void send_file(SOCKET client_socket, const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        char response[] = "HTTP/1.1 404 Not Found\r\n\r\n<h1>404 Not Found</h1>";
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Determine content type
    const char* content_type = "text/plain";
    if (strstr(filepath, ".html")) {
        content_type = "text/html";
    } else if (strstr(filepath, ".css")) {
        content_type = "text/css";
    } else if (strstr(filepath, ".js")) {
        content_type = "application/javascript";
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer and read file
    char* buffer = (char*)malloc(file_size);
    fread(buffer, 1, file_size, file);
    fclose(file);
    
    // Send HTTP headers
    char header[BUFFER_SIZE];
    sprintf(header, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        content_type, file_size);
    
    send(client_socket, header, strlen(header), 0);
    send(client_socket, buffer, file_size, 0);
    
    free(buffer);
}

void handle_scan_request(SOCKET client_socket, const char* query) {
    // Parse target and level from query string
    char target[100] = {0};
    int level = 1;
    
    char* target_ptr = strstr(query, "target=");
    if (target_ptr) {
        sscanf(target_ptr, "target=%[^&]", target);
    }
    
    char* level_ptr = strstr(query, "level=");
    if (level_ptr) {
        level = atoi(level_ptr + 6);
    }
    
    if (strlen(target) == 0) {
        char response[] = "HTTP/1.1 400 Bad Request\r\n\r\n<h1>Target required</h1>";
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    if (level < 1 || level > 4) {
        char response[] = "HTTP/1.1 400 Bad Request\r\n\r\n<h1>Invalid level (1-4)</h1>";
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    // Perform scan
    char ip_addr[INET_ADDRSTRLEN];
    if (!resolve_domain(target, ip_addr)) {
        char response[] = "HTTP/1.1 400 Bad Request\r\n\r\n<h1>Failed to resolve domain</h1>";
        send(client_socket, response, strlen(response), 0);
        return;
    }
    
    struct ScanResult result;
    strcpy(result.target_ip, ip_addr);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(result.timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    result.port_count = 0;
    
    run_nmap_scan(ip_addr, level, &result);
    
    // Format response as JSON (escape fields to produce valid JSON strings)
    char esc_target[256];
    char esc_ip[INET_ADDRSTRLEN*2];
    char esc_time[128];
    char esc_basic[BUFFER_SIZE];
    char esc_medium[BUFFER_SIZE];
    char esc_advanced[BUFFER_SIZE];
    char esc_expert[BUFFER_SIZE];

    json_escape(target, esc_target, sizeof(esc_target));
    json_escape(result.target_ip, esc_ip, sizeof(esc_ip));
    json_escape(result.timestamp, esc_time, sizeof(esc_time));
    json_escape(result.level_descriptions[0], esc_basic, sizeof(esc_basic));
    json_escape(result.level_descriptions[1], esc_medium, sizeof(esc_medium));
    json_escape(result.level_descriptions[2], esc_advanced, sizeof(esc_advanced));
    json_escape(result.level_descriptions[3], esc_expert, sizeof(esc_expert));

    char json_response[BUFFER_SIZE * 4];
    snprintf(json_response, sizeof(json_response),
        "{\n"
        "  \"target\": \"%s\",\n"
        "  \"ip\": \"%s\",\n"
        "  \"timestamp\": \"%s\",\n"
        "  \"level\": %d,\n"
        "  \"result\": {\n"
        "    \"basic\": \"%s\",\n"
        "    \"medium\": \"%s\",\n"
        "    \"advanced\": \"%s\",\n"
        "    \"expert\": \"%s\"\n"
        "  }\n"
        "}\n",
        esc_target, esc_ip, esc_time, level,
        esc_basic, esc_medium, esc_advanced, esc_expert);
    
    // Save JSON to DB (best-effort). We no longer use save_scan_report_file_from_json here.
    if (db_is_ready()) {
        if (db_save_scan(target, result.target_ip, result.timestamp, level, json_response) != 0) {
            fprintf(stderr, "Failed to save scan to DB for %s\n", target);
        }
    } else {
        // DB not available; log a simple message instead.
        printf("[+] Scan result for %s (level %d) not persisted to DB\n", target, level);
    }

    send_response(client_socket, json_response, "application/json");
}