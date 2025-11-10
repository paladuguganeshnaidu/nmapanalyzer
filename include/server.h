#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>

// Function declarations
// Thread entry now uses void* argument to be compatible with _beginthread
void handle_request(void* arg);
void send_response(SOCKET client_socket, const char* content, const char* content_type);
void send_file(SOCKET client_socket, const char* filepath);
void handle_scan_request(SOCKET client_socket, const char* query);

// Start the HTTP server on the given port. Returns 0 on success.
int server_start(int port);

#endif