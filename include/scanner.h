#ifndef SCANNER_H
#define SCANNER_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define MAX_PORTS 100

// Structure to hold scan results
struct ScanResult {
    char target_ip[46];
    char timestamp[100];
    int open_ports[MAX_PORTS];
    int port_count;
    char level_descriptions[4][2048]; // Increased size for better results
};

// Function declarations
int resolve_domain(const char *domain, char *ip);
void run_nmap_scan(const char *target, int level, struct ScanResult *result);
void print_result(const struct ScanResult *result);
void suggest_exploits(const int *ports, int count);
void detect_firewall(const char *target);

#endif