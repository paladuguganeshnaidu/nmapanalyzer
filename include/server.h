#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
#include <winsock2.h>
#else
/* POSIX compatibility types/macros so code can compile on Linux */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

// Function declarations
// Thread entry now uses void* argument to be compatible with pthreads
void handle_request(void* arg);
void send_response(SOCKET client_socket, const char* content, const char* content_type);
void send_file(SOCKET client_socket, const char* filepath);
void handle_scan_request(SOCKET client_socket, const char* query);

// Start the HTTP server on the given port. Returns 0 on success.
int server_start(int port);

#endif