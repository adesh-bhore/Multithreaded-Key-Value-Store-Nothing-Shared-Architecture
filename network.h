#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

int create_server_socket(int port);
int accept_client(int server_fd);
int connect_to_server(const char *host, int port);

int send_all(int sockfd, const char *buf, size_t len);
int recv_line(int sockfd, char *buf, size_t max_len);
void close_socket(int sockfd);

#endif 