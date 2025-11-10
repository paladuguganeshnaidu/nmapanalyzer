#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#define MAX_BUFFER 4096
#define MAX_PORTS 100

// Structure to hold scan results
struct ScanResult {
    char target_ip[INET_ADDRSTRLEN];
    char timestamp[100];
    int open_ports[MAX_PORTS];
    int port_count;
    char level_descriptions[4][500];
};

// Function declarations
int resolve_domain(const char *domain, char *ip);
void run_nmap_scan(const char *target, int level, struct ScanResult *result);
void print_result(const struct ScanResult *result);
void suggest_exploits(const int *ports, int count);
void detect_firewall(const char *target);

/* scanner.c provides run_nmap_scan, resolve_domain, print_result, etc.
 * The standalone main() has been removed so this file can be linked
 * into a single program that selects CLI vs HTTP server in src/main.c.
 */

int resolve_domain(const char *domain, char *ip) {
    struct hostent *he;
    struct in_addr **addr_list;
    
    if ((he = gethostbyname(domain)) == NULL) {
        return 0;
    }
    
    addr_list = (struct in_addr **)he->h_addr_list;
    strcpy(ip, inet_ntoa(*addr_list[0]));
    return 1;
}

void run_nmap_scan(const char *target, int level, struct ScanResult *result) {
    char cmd[MAX_BUFFER];
    FILE *fp;
    char buffer[MAX_BUFFER];
    
    printf("[*] Starting scan level %d on %s\n", level, target);
    
    // Level 1: Basic scanning
    snprintf(cmd, sizeof(cmd), "nmap -sn %s", target);
    fp = popen(cmd, "r");
    if (fp) {
        int len = 0;
        while (fgets(buffer, sizeof(buffer), fp) && len < sizeof(result->level_descriptions[0])-1) {
            int buf_len = strlen(buffer);
            if (len + buf_len < sizeof(result->level_descriptions[0])-1) {
                strcpy(result->level_descriptions[0] + len, buffer);
                len += buf_len;
            } else {
                break;
            }
        }
    pclose(fp);
    }
    
    // Basic port scan
    snprintf(cmd, sizeof(cmd), "nmap -F %s", target);
    fp = popen(cmd, "r");
    if (fp) {
        int len = strlen(result->level_descriptions[0]);
        while (fgets(buffer, sizeof(buffer), fp) && len < sizeof(result->level_descriptions[0])-1) {
            int buf_len = strlen(buffer);
            if (len + buf_len < sizeof(result->level_descriptions[0])-1) {
                strcpy(result->level_descriptions[0] + len, buffer);
                len += buf_len;
            } else {
                break;
            }
        }
    pclose(fp);
    }
    
    if (level >= 2) {
    // Service version detection
    snprintf(cmd, sizeof(cmd), "nmap -sV %s", target);
    fp = popen(cmd, "r");
        if (fp) {
            int len = 0;
            while (fgets(buffer, sizeof(buffer), fp) && len < sizeof(result->level_descriptions[1])-1) {
                int buf_len = strlen(buffer);
                if (len + buf_len < sizeof(result->level_descriptions[1])-1) {
                    strcpy(result->level_descriptions[1] + len, buffer);
                    len += buf_len;
                } else {
                    break;
                }
            }
            pclose(fp);
        }
    }
    
    if (level >= 3) {
        // Vulnerability scripts
    snprintf(cmd, sizeof(cmd), "nmap --script auth,vuln,safe -F %s", target);
    fp = popen(cmd, "r");
        if (fp) {
            int len = 0;
            while (fgets(buffer, sizeof(buffer), fp) && len < sizeof(result->level_descriptions[2])-1) {
                int buf_len = strlen(buffer);
                if (len + buf_len < sizeof(result->level_descriptions[2])-1) {
                    strcpy(result->level_descriptions[2] + len, buffer);
                    len += buf_len;
                } else {
                    break;
                }
            }
            pclose(fp);
        }
    }
    
    if (level >= 4) {
        // Expert scan
    snprintf(cmd, sizeof(cmd), "nmap --script vuln --randomize-hosts --scan-delay 5 -F %s", target);
    fp = popen(cmd, "r");
        if (fp) {
            int len = 0;
            while (fgets(buffer, sizeof(buffer), fp) && len < sizeof(result->level_descriptions[3])-1) {
                int buf_len = strlen(buffer);
                if (len + buf_len < sizeof(result->level_descriptions[3])-1) {
                    strcpy(result->level_descriptions[3] + len, buffer);
                    len += buf_len;
                } else {
                    break;
                }
            }
            pclose(fp);
        }
    }
    
    // Get open ports for exploit suggestions
    snprintf(cmd, sizeof(cmd), "nmap -p- --open %s | findstr /i \"open\" | for /f \"tokens=1 delims=/\" %%i in ('more') do @echo %%i", target);
    fp = popen(cmd, "r");
    if (fp) {
        char port_str[10];
        while (fgets(port_str, sizeof(port_str), fp) && result->port_count < MAX_PORTS) {
            port_str[strcspn(port_str, "\n")] = 0; // Remove newline
            port_str[strcspn(port_str, "\r")] = 0; // Remove carriage return
            if (strlen(port_str) > 0) {
                result->open_ports[result->port_count++] = atoi(port_str);
            }
        }
            pclose(fp);
    }
}

void print_result(const struct ScanResult *result) {
    printf("\n==========================================\n");
    printf("         NMAP AUTOMATOR REPORT           \n");
    printf("==========================================\n");
    printf("Target IP: %s\n", result->target_ip);
    printf("Timestamp: %s\n", result->timestamp);
    printf("\n[*] BASIC INFORMATION GATHERING\n");
    printf("----------------------------------------\n");
    printf("%s\n", result->level_descriptions[0]);

    if (strlen(result->level_descriptions[1]) > 0) {
        printf("\n[*] MEDIUM INTENSITY SCANNING\n");
        printf("----------------------------------------\n");
        printf("%s\n", result->level_descriptions[1]);
    }

    if (strlen(result->level_descriptions[2]) > 0) {
        printf("\n[*] ADVANCED SCANNING TECHNIQUES\n");
        printf("----------------------------------------\n");
        printf("%s\n", result->level_descriptions[2]);
    }

    if (strlen(result->level_descriptions[3]) > 0) {
        printf("\n[*] EXPERT-LEVEL ANALYSIS\n");
        printf("----------------------------------------\n");
        printf("%s\n", result->level_descriptions[3]);
    }
}

void suggest_exploits(const int *ports, int count) {
    if (count == 0) {
        printf("\n[*] No open ports detected to suggest exploits.\n");
        return;
    }

    printf("\n[*] EXPLOITATION TOOL SUGGESTIONS\n");
    printf("----------------------------------------\n");
    printf("Found %d open ports. Exploitation tool suggestions:\n\n", count);

    for (int i = 0; i < count; i++) {
        int port = ports[i];
        printf("Port %d: ", port);
        
        switch(port) {
            case 21:
                printf("FTP service\n");
                printf("  - hydra -l admin -P passwords.txt ftp://TARGET\n");
                printf("  - medusa -u admin -P passwords.txt -h TARGET -M ftp\n");
                break;
            case 22:
                printf("SSH service\n");
                printf("  - hydra -l root -P passwords.txt ssh://TARGET\n");
                printf("  - medusa -u root -P passwords.txt -h TARGET -M ssh\n");
                break;
            case 23:
                printf("Telnet service\n");
                printf("  - telnet TARGET\n");
                break;
            case 25:
                printf("SMTP service\n");
                printf("  - smtp-user-enum -M VRFY -U users.txt -t TARGET\n");
                break;
            case 53:
                printf("DNS service\n");
                printf("  - dnsrecon -d TARGET\n");
                break;
            case 80:
                printf("HTTP service\n");
                printf("  - nikto -h http://TARGET\n");
                printf("  - dirb http://TARGET wordlists.txt\n");
                break;
            case 443:
                printf("HTTPS service\n");
                printf("  - nikto -h https://TARGET\n");
                printf("  - sslscan TARGET:443\n");
                break;
            case 445:
                printf("SMB service\n");
                printf("  - smbclient //TARGET/share\n");
                printf("  - enum4linux TARGET\n");
                break;
            case 3389:
                printf("RDP service\n");
                printf("  - ncrack -vv --user administrator -P passwords.txt rdp://TARGET\n");
                break;
            default:
                printf("General purpose\n");
                printf("  - nmap -sV -p %d TARGET\n", port);
                printf("  - nc TARGET %d\n", port);
        }
        printf("\n");
    }
}

void detect_firewall(const char *target) {
    printf("\n[*] FIREWALL DETECTION\n");
    printf("----------------------\n");
    
    char cmd[MAX_BUFFER];
    FILE *fp;
    
    // Run SYN scan vs Connect scan
    snprintf(cmd, sizeof(cmd), "nmap -sS -p 80,443 %s", target);
    fp = popen(cmd, "r");
    if (fp) {
        char buffer[MAX_BUFFER];
        printf("SYN scan results:\n");
        while (fgets(buffer, sizeof(buffer), fp)) {
            printf("  %s", buffer);
        }
            pclose(fp);
    }
    
    snprintf(cmd, sizeof(cmd), "nmap -sT -p 80,443 %s", target);
    fp = popen(cmd, "r");
    if (fp) {
        char buffer[MAX_BUFFER];
        printf("Connect scan results:\n");
        while (fgets(buffer, sizeof(buffer), fp)) {
            printf("  %s", buffer);
        }
    pclose(fp);
    }
    
    printf("\n[*] Compare results above to detect potential firewall filtering.\n");
}