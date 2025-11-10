#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include "scanner.h"
#include "server.h"
#include "db.h"
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 4096

// Function declarations
void __cdecl handle_request(void* arg);
void send_response(SOCKET client_socket, const char* content, const char* content_type);
void send_file(SOCKET client_socket, const char* filepath);
void handle_scan_request(SOCKET client_socket, const char* query);
#include <time.h>

int server_start(int port) {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);
    // Initialize DB (use data/scans.db)
    if (!db_init("data/scans.db")) {
        fprintf(stderr, "Warning: failed to initialize DB, continuing without persistence.\n");
    }
    
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed to initialize Winsock.\n");
        return 1;
    }
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        WSACleanup();
        return 1;
    }
    
    // Prepare sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    
    // Listen
    if (listen(server_socket, 3) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        closesocket(server_socket);
        WSACleanup();
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
        
        // Handle request in a separate thread
        uintptr_t th = _beginthread(handle_request, 0, (void*)(uintptr_t)client_socket);
        if (th == (uintptr_t)-1) {
            printf("Thread creation failed.\n");
            closesocket(client_socket);
        }
    }
    
    closesocket(server_socket);
    WSACleanup();
    return 0;
}

void __cdecl handle_request(void* arg) {
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
        "Content-Length: %d\r\n"
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
    
    // Format response as JSON
    char json_response[BUFFER_SIZE * 4];
    sprintf(json_response, 
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
        target, result.target_ip, result.timestamp, level,
        result.level_descriptions[0], 
        result.level_descriptions[1], 
        result.level_descriptions[2], 
        result.level_descriptions[3]);
    
    // Save JSON to DB (best-effort)
    if (db_is_ready()) {
        int rc = db_save_scan(target, result.target_ip, result.timestamp, level, json_response);
        if (rc == 0) {
            printf("[+] Scan saved to DB: %s @ %s (level %d)\n", target, result.timestamp, level);
        } else {
            fprintf(stderr, "[-] db_save_scan failed (rc=%d) for %s\n", rc, target);
        }
    } else {
        printf("[!] DB not ready — scan not persisted: %s (level %d)\n", target, level);
    }

    send_response(client_socket, json_response, "application/json");
}