#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "constants.h"
#include "protocol.h"
#include "commands.h"

/* Global flag for graceful shutdown on Ctrl+C */
static volatile sig_atomic_t stop_requested = 0;

static void handle_sigint(int sig) {
    (void)sig;
    stop_requested = 1;
}

int main(int argc, char *argv[]) {
    int opt;
    int port = DEFAULT_ES_PORT;
    
    /* Ignore SIGPIPE */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, NULL) == -1) {
        perror("sigaction");
        return 1;
    }
    
    /* Set up SIGINT handler for graceful shutdown */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handle_sigint;
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction");
        return 1;
    }
    
    /* Parse command line arguments */
    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-p ESport] [-v]\n", argv[0]);
                return 1;
        }
    }
    
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    printf("Event Reservation Server (ES)\n");
    printf("Port: %s | Verbose: %s\n\n", port_str, verbose ? "ON" : "OFF");
    
    struct addrinfo hints, *res;
    int ufd, tfd;
    int errcode;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    
    if ((errcode = getaddrinfo(NULL, port_str, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo (UDP): %s\n", gai_strerror(errcode));
        return 1;
    }
    
    ufd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (ufd == -1) {
        perror("socket (UDP)");
        return 1;
    }
    
    if (bind(ufd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind (UDP)");
        return 1;
    }
    
    freeaddrinfo(res);
    printf("UDP server listening on port %s\n", port_str);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    
    if ((errcode = getaddrinfo(NULL, port_str, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo (TCP): %s\n", gai_strerror(errcode));
        return 1;
    }
    
    tfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (tfd == -1) {
        perror("socket (TCP)");
        return 1;
    }
    
    /* Allow port reuse */
    int optval = 1;
    if (setsockopt(tfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt");
        return 1;
    }
    
    if (bind(tfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind (TCP)");
        return 1;
    }
    
    if (listen(tfd, 5) == -1) {
        perror("listen");
        return 1;
    }
    
    freeaddrinfo(res);
    printf("TCP server listening on port %s\n", port_str);
    

    fd_set inputs, testfds;
    int max_fd;
    
    FD_ZERO(&inputs);
    FD_SET(ufd, &inputs);
    FD_SET(tfd, &inputs);
    
    max_fd = (ufd > tfd) ? ufd : tfd;
    
    printf("\nServer ready. Waiting for connections...\n\n");
    
    while (1) {
        /* Check if Ctrl+C was pressed */
        if (stop_requested) {
            printf("\nShutdown requested. Closing server...\n");
            break;
        }
        
        testfds = inputs;
        
        int out_fds = select(max_fd + 1, &testfds, NULL, NULL, NULL);
        
        if (out_fds == -1) {
            perror("select");
            break;
        }
        
        /* Check UDP socket */
        if (FD_ISSET(ufd, &testfds)) {
            handle_udp_request(ufd);
        }
        
        /* Check TCP socket*/
        if (FD_ISSET(tfd, &testfds)) {
            struct sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);
            
            int newfd = accept(tfd, (struct sockaddr *)&client_addr, &addrlen);
            if (newfd == -1) {
                perror("accept");
                continue;
            }
            
            handle_tcp_connection(newfd, &client_addr);
        }
    }
    
    /* Cleanup */
    close(ufd);
    close(tfd);
    
    return 0;
}
