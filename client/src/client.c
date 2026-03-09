#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include "constants.h"
#include "protocol.h"
#include "commands.h"

/* Initialize global server connection variables */
char *es_ip = "127.0.0.1";
char *es_port = NULL;
static char default_port[16];

/* Flag for graceful shutdown on Ctrl+C */
static volatile sig_atomic_t stop_requested = 0;

/**
 * handle_sigint - Signal handler for SIGINT (Ctrl+C)
 * 
 * Sets flag to terminate main loop gracefully
 */
static void handle_sigint(int sig) {
    (void)sig;
    stop_requested = 1;
}

/**
 * parse_command_type - Identify command type from user input
 * @input: Raw command string from user
 * 
 * Extracts first word, converts to lowercase, and matches against
 * known commands. Returns CMD_UNKNOWN if no match.
 */
static int parse_command_type(const char *input) {
    char cmd[CMD_MAX_LEN];
    char cmd_lower[CMD_MAX_LEN];
    size_t i = 0;

    /* Extract first word (command name) */
    while (input[i] && !isspace((unsigned char)input[i]) && i < sizeof(cmd) - 1) {
        cmd[i] = input[i];
        cmd_lower[i] = tolower((unsigned char)input[i]);
        i++;
    }
    cmd[i] = '\0';
    cmd_lower[i] = '\0';

    if (i == 0) return CMD_UNKNOWN;

    /* Match command (case-insensitive) */
    switch (cmd_lower[0]) {
        case 'l':
            if (strcmp(cmd_lower, "login") == 0) return CMD_LOGIN;
            if (strcmp(cmd_lower, "list") == 0) return CMD_LIST;
            if (strcmp(cmd_lower, "logout") == 0) return CMD_LOGOUT;
            break;
        case 'c':
            if (strcmp(cmd_lower, "changepass") == 0) return CMD_CHANGEPASS;
            if (strcmp(cmd_lower, "create") == 0) return CMD_CREATE;
            if (strcmp(cmd_lower, "close") == 0) return CMD_CLOSE;
            break;
        case 'u':
            if (strcmp(cmd_lower, "unregister") == 0) return CMD_UNREGISTER;
            break;
        case 'e':
            if (strcmp(cmd_lower, "exit") == 0) return CMD_EXIT;
            break;
        case 'm':
            if (strcmp(cmd_lower, "myevents") == 0 || strcmp(cmd_lower, "mye") == 0) return CMD_MYEVENTS;
            if (strcmp(cmd_lower, "myreservations") == 0 || strcmp(cmd_lower, "myr") == 0) return CMD_MYRESERVATIONS;
            break;
        case 's':
            if (strcmp(cmd_lower, "show") == 0) return CMD_SHOW;
            break;
        case 'r':
            if (strcmp(cmd_lower, "reserve") == 0) return CMD_RESERVE;
            break;
    }
    return CMD_UNKNOWN;
}

int main(int argc, char *argv[]) {
    char command[CMD_MAX_LEN];
    int cmdType;
    int opt;

    /* Set default port from GROUP_NUMBER */
    snprintf(default_port, sizeof(default_port), "%d", DEFAULT_ES_PORT);
    es_port = default_port;

    /* Parse command line arguments: -n ESIP -p ESport */
    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                es_ip = optarg;
                break;
            case 'p':
                es_port = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-n ESIP] [-p ESport]\n", argv[0]);
                return 1;
        }
    }

    /* Set up SIGINT handler for graceful shutdown */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("Event Reservation Client\n");
    printf("Connecting to ES at %s:%s\n\n", es_ip, es_port);

    while (1) {
        /* Check if Ctrl+C was pressed */
        if (stop_requested) {
            printf("\nShutdown requested. Exiting...\n");
            return 0;
        }

        printf("Enter command: ");
        if (!fgets(command, sizeof(command), stdin)) {
            /* Check if interrupted by signal */
            if (stop_requested) {
                printf("\nShutdown requested. Exiting...\n");
                return 0;
            }
            /* EOF or error */
            printf("\n");
            return 1;
        }

        command[strcspn(command, "\n")] = '\0'; /* Remove newline */

        cmdType = parse_command_type(command);

        switch (cmdType) {

            case CMD_LOGIN: /* login UID password */
                handle_login_command(command);
                break;
            case CMD_CHANGEPASS: /* changePass oldPassword newPassword */
                handle_change_pass(command);
                break;
            case CMD_UNREGISTER: /* unregister */
                handle_unregister_command();
                break;
            case CMD_LOGOUT: /* logout */
                handle_logout_command();
                break;
            case CMD_EXIT: /* exit */
                if (handle_exit_command() == RC_OK) {
                    return 0;
                }
                break;
            case CMD_CREATE: /* create name event_fname event_date num_attendees */
                handle_create_command(command);
                break;
            case CMD_CLOSE: /* close EID */
                handle_close_command(command);
                break;
            case CMD_MYEVENTS: /* myevents / mye */
                handle_myevents_command();
                break;
            case CMD_LIST: /* list */
                handle_list_command();
                break;
            case CMD_SHOW: /* show EID */
                handle_show_command(command);
                break;
            case CMD_RESERVE: /* reserve EID value */
                handle_reserve_command(command);
                break;
            case CMD_MYRESERVATIONS: /* myreservations / myr */
                handle_myreservations_command();
                break;
            default: /* unknown command */
                printf("Unknown command. Please try again.\n");
        }
    }

    return 0;
}