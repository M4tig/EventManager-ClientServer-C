#ifndef RC_SERVER_PROTOCOL_H
#define RC_SERVER_PROTOCOL_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Global server settings */
extern int verbose;
extern char port_str[16];

/* Write all bytes to file descriptor */
ssize_t write_all(int fd, const char *buf, ssize_t nbytes);

/* Read a line (until '\n') from socket */
ssize_t read_line(int fd, char *buf, size_t maxlen);

/* Read exact number of bytes from socket */
ssize_t read_all(int fd, char *buf, ssize_t nbytes);

/* Print verbose info about incoming request */
void verbose_print(const char *protocol, const char *ip, int port, 
                   const char *req_type, const char *uid);

/* Get client IP and port from sockaddr_in */
void get_client_info(struct sockaddr_in *addr, char *ip, int *port);

#endif
