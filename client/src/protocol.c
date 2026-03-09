#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#include "constants.h"
#include "protocol.h"

/**
 * write_all - Ensures all bytes are written to socket/file descriptor
 * 
 * Handles partial writes by looping until all data is sent
 */
ssize_t write_all(int fd, const char *buf, ssize_t nbytes) {
    ssize_t nleft = nbytes;
    const char *ptr = buf;

    while (nleft > 0) {
        ssize_t nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0) return -1;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return nbytes;
}

/**
 * send_udp_request - Send UDP request and wait for response
 * 
 * Creates UDP socket, sets receive timeout, sends request to server,
 * and receives response. Handles DNS resolution automatically.
 */
int send_udp_request(const char *ip, const char *port, 
                     const char *request, char *response, 
                     size_t response_size){

    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res = NULL;
    struct sockaddr_in addr;

    // Create UDP socket
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1){
        return -1;
    }

    // Set receive timeout to avoid blocking indefinitely
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(fd);
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(ip, port, &hints, &res);
    if (errcode != 0) {
        close(fd);
        return -1;
    }
    n = sendto(fd, request, strlen(request), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    addrlen = sizeof(addr);
    n = recvfrom(fd, response, response_size - 1, 0, 
                 (struct sockaddr*)&addr, &addrlen);
    if (n == -1) {
        freeaddrinfo(res);
        close(fd);
        return -1;
    }
    response[n] = '\0';

    freeaddrinfo(res);
    close(fd);
    
    return n;
}


int connect_tcp(const char *ip, const char *port) {
    struct addrinfo hints, *res = NULL;
    int fd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, port, &hints, &res) != 0) {
        return -1;
    }

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) {
        freeaddrinfo(res);
        return -1;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);
    return fd;
}

int send_tcp_request(const char *ip, const char *port, 
                     const char *request, char *response, 
                     size_t response_size) {

    /* Ignore SIGPIPE so write failures return -1 instead of killing the process */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) return -1;

    int fd = connect_tcp(ip, port);
    if (fd == -1) return -1;

    if (write_all(fd, request, strlen(request)) == -1) {
        close(fd);
        return -1;
    }

    /* Read until EOF or buffer full (like the provided read_tcp_socket) */
    size_t bytes_read = 0;
    memset(response, 0, response_size);

    while (bytes_read < response_size - 1) {
        ssize_t n = read(fd, response + bytes_read, response_size - 1 - bytes_read);
        if (n < 0) {
            close(fd);
            return -1;
        }
        if (n == 0) break; /* Connection closed */
        bytes_read += (size_t)n;
    }

    response[bytes_read] = '\0';
    close(fd);
    return (int)bytes_read;
}

int send_tcp_request_line(const char *ip, const char *port, 
                          const char *request, char *response, 
                          size_t response_size) {

    /* Ignore SIGPIPE for safety */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) return -1;

    int fd = connect_tcp(ip, port);
    if (fd == -1) return -1;

    if (write_all(fd, request, strlen(request)) == -1) {
        close(fd);
        return -1;
    }

    /* Line-oriented read: stop at newline or when buffer is almost full */
    size_t bytes_read = 0;
    memset(response, 0, response_size);

    while (bytes_read < response_size - 1) {
        ssize_t n = read(fd, response + bytes_read, 1);
        if (n < 0) {
            close(fd);
            return -1;
        }
        if (n == 0) break; /* Connection closed */

        bytes_read += (size_t)n;
        if (response[bytes_read - 1] == '\n') break;
    }

    response[bytes_read] = '\0';
    close(fd);
    return (int)bytes_read;
}