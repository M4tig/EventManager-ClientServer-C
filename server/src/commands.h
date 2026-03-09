#ifndef RC_SERVER_COMMANDS_H
#define RC_SERVER_COMMANDS_H

#include <sys/socket.h>
#include <netinet/in.h>

/* UDP command handlers */
void handle_udp_request(int ufd);

/* TCP command handlers */
void handle_tcp_connection(int newfd, struct sockaddr_in *client_addr);

#endif
