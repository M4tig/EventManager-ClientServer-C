#ifndef RC_CLIENT_PROTOCOL_H
#define RC_CLIENT_PROTOCOL_H

#include <stddef.h>
#include <sys/types.h>

/* Global variables for server connection */
extern char *es_ip;
extern char *es_port;

/**
 * write_all - Write all bytes to a file descriptor
 * @fd: File descriptor to write to
 * @buf: Buffer containing data to write
 * @nbytes: Number of bytes to write
 *
 * Ensures all bytes are written, looping until complete or error
 * Returns: Total bytes written on success, -1 on error
 */
ssize_t write_all(int fd, const char *buf, ssize_t nbytes);

/**
 * send_udp_request - Send a request via UDP and receive response
 * @ip: IP address of server
 * @port: Port number (as string)
 * @request: Request message to send
 * @response: Buffer to store response
 * @response_size: Size of response buffer
 *
 * Returns: Number of bytes received, -1 on error
 */
int send_udp_request(const char *ip, const char *port, 
                     const char *request, char *response, 
                     size_t response_size);

/**
 * connect_tcp - Establish TCP connection to server
 * @ip: IP address of server
 * @port: Port number (as string)
 *
 * Returns: Socket file descriptor on success, -1 on error
 */
int connect_tcp(const char *ip, const char *port);

/**
 * send_tcp_request - Send request via TCP and receive full response
 * @ip: IP address of server
 * @port: Port number (as string)
 * @request: Request message to send
 * @response: Buffer to store response
 * @response_size: Size of response buffer
 *
 * Reads until EOF or buffer full. Closes connection automatically.
 * Returns: Number of bytes received, -1 on error
 */
int send_tcp_request(const char *ip, const char *port, 
                     const char *request, char *response, 
                     size_t response_size);

/**
 * send_tcp_request_line - Send request via TCP and receive single line
 * @ip: IP address of server
 * @port: Port number (as string)
 * @request: Request message to send
 * @response: Buffer to store response
 * @response_size: Size of response buffer
 *
 * Reads response line-by-line until newline. Closes connection automatically.
 * Returns: Number of bytes received, -1 on error
 */
int send_tcp_request_line(const char *ip, const char *port, 
                          const char *request, char *response, 
                          size_t response_size);

#endif
