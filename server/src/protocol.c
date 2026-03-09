#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "constants.h"
#include "protocol.h"

/* Global server settings */
int verbose = 0;
char port_str[16];

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

ssize_t read_line(int fd, char *buf, size_t maxlen) {
    size_t n = 0;
    
    while (n < maxlen - 1) {
        ssize_t rc = read(fd, buf + n, 1);
        if (rc < 0) return -1;
        if (rc == 0) break;  /* EOF */
        n++;
        if (buf[n - 1] == '\n') break;
    }
    buf[n] = '\0';
    return (ssize_t)n;
}

ssize_t read_all(int fd, char *buf, ssize_t nbytes) {
    ssize_t nleft = nbytes;
    char *ptr = buf;

    while (nleft > 0) {
        ssize_t nread = read(fd, ptr, nleft);
        if (nread < 0) return -1;
        if (nread == 0) break;  /* EOF */
        nleft -= nread;
        ptr += nread;
    }
    return nbytes - nleft;
}

void verbose_print(const char *protocol, const char *ip, int port, 
                   const char *req_type, const char *uid) {
    if (!verbose) return;
    printf("[%s] %s:%d | Request: %s", protocol, ip, port, req_type);
    if (uid && strlen(uid) > 0) {
        printf(" | UID: %s", uid);
    }
    printf("\n");
}

void get_client_info(struct sockaddr_in *addr, char *ip, int *port) {
    inet_ntop(AF_INET, &addr->sin_addr, ip, INET_ADDRSTRLEN);
    *port = ntohs(addr->sin_port);
}
