#include "network.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* ── Platform-specific includes ──────────────────────────────────────────── */
#ifdef _WIN32
    /* Windows (Winsock) */
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")  // Link with Winsock library
    
    // Winsock initialization flag
    static int winsock_initialized = 0;
    
    static void init_winsock(void) {
        if (!winsock_initialized) {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                fprintf(stderr, "WSAStartup failed\n");
                exit(1);
            }
            winsock_initialized = 1;
        }
    }
#else
    /* Linux/Unix (POSIX sockets) */
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    
    #define init_winsock()  // No-op on Unix
#endif

/* ══════════════════════════════════════════════════════════════════════════
 * SERVER OPERATIONS
 * ══════════════════════════════════════════════════════════════════════════ */

int create_server_socket(int port) {
    init_winsock();
    
    /* Step 1: Create TCP socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    /* Step 2: Set SO_REUSEADDR option
     * Allows immediate port reuse after server restart
     * Without this, you'd get "Address already in use" error */
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const char*)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close_socket(sockfd);
        return -1;
    }
    
    /* Step 3: Bind socket to address
     * struct sockaddr_in holds IP address and port */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;              // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;      // Listen on all interfaces (0.0.0.0)
    addr.sin_port = htons(port);            // Convert port to network byte order
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close_socket(sockfd);
        return -1;
    }
    
    /* Step 4: Start listening
     * Backlog of 10 = max 10 pending connections in queue */
    if (listen(sockfd, 10) < 0) {
        perror("listen");
        close_socket(sockfd);
        return -1;
    }
    
    printf("[Network] Server socket created on port %d\n", port);
    return sockfd;
}

int accept_client(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    /* Accept blocks until a client connects */
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        perror("accept");
        return -1;
    }
    
    /* Get client IP address for logging */
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    printf("[Network] Client connected from %s:%d (fd=%d)\n", 
           client_ip, ntohs(client_addr.sin_port), client_fd);
    
    return client_fd;
}

/* ══════════════════════════════════════════════════════════════════════════
 * CLIENT OPERATIONS
 * ══════════════════════════════════════════════════════════════════════════ */

int connect_to_server(const char *host, int port) {
    init_winsock();
    
    /* Step 1: Create TCP socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    /* Step 2: Resolve hostname to IP address */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Try to convert host as IP address first
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        // Not an IP, try DNS lookup
        struct hostent *he = gethostbyname(host);
        if (he == NULL) {
            fprintf(stderr, "gethostbyname failed for %s\n", host);
            close_socket(sockfd);
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    /* Step 3: Connect to server */
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close_socket(sockfd);
        return -1;
    }
    
    printf("[Network] Connected to %s:%d\n", host, port);
    return sockfd;
}

/* ══════════════════════════════════════════════════════════════════════════
 * DATA TRANSFER
 * ══════════════════════════════════════════════════════════════════════════ */

int send_all(int sockfd, const char *buf, size_t len) {
    size_t total_sent = 0;
    
    /* Loop until all data is sent
     * send() may send less than requested (partial send) */
    while (total_sent < len) {
        int n = send(sockfd, buf + total_sent, len - total_sent, 0);
        if (n < 0) {
            perror("send");
            return -1;
        }
        if (n == 0) {
            // Connection closed
            return -1;
        }
        total_sent += n;
    }
    
    return 0;
}

int recv_line(int sockfd, char *buf, size_t max_len) {
    size_t pos = 0;
    
    /* Read one byte at a time until we find \r\n */
    while (pos < max_len - 1) {
        char c;
        ssize_t n = recv(sockfd, &c, 1, 0);
        
        if (n < 0) {
            perror("recv");
            return -1;
        }
        if (n == 0) {
            // Connection closed - but if we have data, return it
            if (pos > 0) {
                buf[pos] = '\0';
                return pos;
            }
            return 0;
        }
        
        buf[pos++] = c;
        
        /* Check for \r\n terminator */
        if (pos >= 2 && buf[pos-2] == '\r' && buf[pos-1] == '\n') {
            buf[pos-2] = '\0';  // Null-terminate, removing \r\n
            return pos - 2;     // Return length without \r\n
        }
        
        /* Also handle just \n (Unix style) */
        if (c == '\n') {
            buf[pos-1] = '\0';  // Remove \n
            return pos - 1;
        }
    }
    
    /* Buffer full without finding terminator */
    buf[max_len-1] = '\0';
    return pos;
}

void close_socket(int sockfd) {
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}