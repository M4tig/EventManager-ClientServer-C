#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "constants.h"
#include "protocol.h"

#define UID_LEN    6
#define PASS_LEN   8
#define RESP_MAX 128

/* Global login state */
int logged_in = 0;
char current_uid[UID_LEN + 1] = "";
char current_pass[PASS_LEN + 1] = "";

/**
 * clear_local_login - Clear login state
 * 
 * Resets user credentials in local memory
 */
static void clear_local_login(void) {
    logged_in = 0;
    current_uid[0] = '\0';
    current_pass[0] = '\0';
}

/**
 * valid_uid - Check if UID format is valid
 * @uid: User ID to validate
 * 
 * UID must be exactly 6 digits
 */
static int valid_uid(const char *uid) {
    if (strlen(uid) != UID_LEN) return 0;
    for (int i = 0; i < UID_LEN; i++)
        if (!isdigit((unsigned char)uid[i])) return 0;
    return 1;
}

/**
 * valid_password - Check if password format is valid
 * @pw: Password to validate
 * 
 * Password must be exactly 8 alphanumeric characters
 */
static int valid_password(const char *pw) {
    if (strlen(pw) != PASS_LEN) return 0;
    for (int i = 0; i < PASS_LEN; i++)
        if (!isalnum((unsigned char)pw[i])) return 0;
    return 1;
}

/**
 * valid_eid_int - Check if event ID is in valid range
 * @eid: Event ID to validate
 * 
 * EID must be between 1 and 999
 */
static int valid_eid_int(int eid) {
    return (eid >= 1 && eid <= 999);
}

/**
 * valid_people_int - Check if number of people is valid
 * @people: Number of people to validate
 * 
 * Must be between 1 and 999
 */
static int valid_people_int(int people) {
    return (people >= 1 && people <= 999);
}

/**
 * valid_event_name - Check if event name is valid
 * @name: Event name to validate
 * 
 * Must be 1-10 alphanumeric characters
 */
static int valid_event_name(const char *name) {
    if (strlen(name) == 0 || strlen(name) > 10) return 0;
    for (int i = 0; name[i]; i++)
        if (!isalnum((unsigned char)name[i])) return 0;
    return 1;
}

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * valid_event_date - Check if event date is valid
 * @date: Date string in format "dd-mm-yyyy"
 */
static int valid_event_date(const char *date) {
    int day, month, year;
    int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (strlen(date) != 10) return 0;
    if (date[2] != '-' || date[5] != '-') return 0;
    if (sscanf(date, "%d-%d-%d", &day, &month, &year) != 3) return 0;
    
    /* Validate ranges */
    if (month < 1 || month > 12) return 0;
    if (year < 0 || year > 2099) return 0;
    
    /* Adjust February for leap years */
    if (month == 2 && is_leap_year(year)) {
        days_in_month[2] = 29;
    }
    
    /* Validate day based on the month */
    if (day < 1 || day > days_in_month[month]) return 0;
    
    return 1;
}

/**
 * valid_filename - Check if filename is valid
 * @fname: Filename to validate
 * 
 * Max 24 chars, alphanumeric with '-', '_', '.' and 3-letter extension
 */
static int valid_filename(const char *fname) {
    int len = strlen(fname);
    if (len == 0 || len > 24) return 0;
    
    /* Find and validate extension */
    int dot_pos = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (fname[i] == '.') {
            dot_pos = i;
            break;
        }
    }
    
    if (dot_pos == -1 || dot_pos == 0 || len - dot_pos != 4) return 0;
    
    /* Check extension is 3 letters */
    for (int i = dot_pos + 1; i < len; i++)
        if (!isalpha((unsigned char)fname[i])) return 0;
    
    /* Check rest of filename is alphanumeric + '-', '_', '.' */
    for (int i = 0; i < dot_pos; i++) {
        char c = fname[i];
        if (!isalnum((unsigned char)c) && c != '-' && c != '_') return 0;
    }
    
    return 1;
}

void handle_login_command(const char *command_line)
{
    char uid[32], pw[32];
    char req[128];
    char resp[RESP_MAX];
    int n;
    char tag[8], status[8];

    if (logged_in) {
        printf("You are already logged in as %s. Please logout first.\n", current_uid);
        return;
    }

    if (sscanf(command_line, "login %31s %31s", uid, pw) != 2) {
        printf("Usage: login UID password\n");
        return;
    }

    if (!valid_uid(uid)) {
        printf("Invalid UID. It must be exactly 6 digits.\n");
        return;
    }
    if (!valid_password(pw)) {
        printf("Invalid password. It must be 8 alphanumeric characters.\n");
        return;
    }

    /* Send UDP login request to server */
    snprintf(req, sizeof(req), "LIN %s %s\n", uid, pw);

    n = send_udp_request(es_ip, es_port, req, resp, sizeof(resp));
    if (n < 0) {
        perror("send_udp_request (login)");
        return;
    }

    if (sscanf(resp, "%7s %7s", tag, status) != 2 || strcmp(tag, "RLI") != 0) {
        printf("Protocol error on login reply: %s\n", resp);
        return;
    }

    /* Handle server response */
    if (strcmp(status, "OK") == 0) {
        printf("Login successful. Welcome, %s.\n", uid);
        logged_in = 1;
        strncpy(current_uid, uid, UID_LEN + 1);
        strncpy(current_pass, pw, PASS_LEN + 1);
    } else if (strcmp(status, "NOK") == 0) {
        printf("Login failed: incorrect UID or password.\n");
    } else if (strcmp(status, "REG") == 0) {
        printf("New user %s registered and logged in.\n", uid);
        logged_in = 1;
        strncpy(current_uid, uid, UID_LEN + 1);
        strncpy(current_pass, pw, PASS_LEN + 1);
    } else if (strcmp(status, "ERR") == 0) {
        printf("Login failed: server reported an error.\n");
    } else {
        printf("Login failed: unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}

void handle_change_pass(const char *command_line)
{
    char old_pw[32], new_pw[32];
    char req[256];
    char resp[RESP_MAX];
    int n;
    char tag[8], status[8];

    if (!logged_in) {
        printf("You must be logged in to change password.\n");
        return;
    }

    if (sscanf(command_line, "changePass %31s %31s", old_pw, new_pw) != 2 &&
        sscanf(command_line, "changepass %31s %31s", old_pw, new_pw) != 2) {
        printf("Usage: changePass oldPassword newPassword\n");
        return;
    }

    if (!valid_password(old_pw)) {
        printf("Invalid old password. It must be 8 alphanumeric characters.\n");
        return;
    }
    if (!valid_password(new_pw)) {
        printf("Invalid new password. It must be 8 alphanumeric characters.\n");
        return;
    }

    if (strcmp(old_pw, current_pass) != 0) {
        printf("Error: old password does not match current password.\n");
        return;
    }

    /* Send TCP password change request to server */
    snprintf(req, sizeof(req), "CPS %s %s %s\n", current_uid, old_pw, new_pw);

    n = send_tcp_request_line(es_ip, es_port, req, resp, sizeof(resp));
    if (n < 0) {
        perror("send_tcp_request_line (changePass)");
        return;
    }

    if (sscanf(resp, "%7s %7s", tag, status) != 2 || strcmp(tag, "RCP") != 0) {
        printf("Protocol error on changePass reply: %s\n", resp);
        return;
    }

    if (strcmp(status, "OK") == 0) {
        printf("Password changed successfully.\n");
        strncpy(current_pass, new_pw, PASS_LEN + 1);
    } else if (strcmp(status, "NOK") == 0) {
        printf("Password change failed: incorrect password.\n");
    } else if (strcmp(status, "NLG") == 0) {
        printf("Password change failed: user not logged in.\n");
        clear_local_login();
    } else if (strcmp(status, "NID") == 0) {
        printf("Password change failed: user does not exist.\n");
    } else if (strcmp(status, "ERR") == 0) {
        printf("Password change failed: server reported protocol error.\n");
    } else {
        printf("Password change failed: unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}

void handle_unregister_command(void)
{
    char req[64];
    char resp[RESP_MAX];
    int n;
    char tag[8], status[8];

    if (!logged_in) {
        printf("No user is currently logged in. Use login first.\n");
        return;
    }

    /* Send UDP unregister request to server */
    snprintf(req, sizeof(req), "UNR %s %s\n", current_uid, current_pass);

    n = send_udp_request(es_ip, es_port, req, resp, sizeof(resp));
    if (n < 0) {
        perror("send_udp_request (unregister)");
        return;
    }

    if (sscanf(resp, "%7s %7s", tag, status) != 2 || strcmp(tag, "RUR") != 0) {
        printf("Protocol error on unregister reply: %s\n", resp);
        return;
    }

    /* Handle server response */
    if (strcmp(status, "OK") == 0) {
        printf("Unregister successful. User %s removed.\n", current_uid);
        logged_in = 0;
        current_uid[0] = '\0';
        current_pass[0] = '\0';
    } else if (strcmp(status, "NOK") == 0) {
        printf("Unregister failed: user not logged in on server.\n");
        logged_in = 0;
        current_uid[0] = '\0';
        current_pass[0] = '\0';
    } else if (strcmp(status, "UNR") == 0) {
        printf("Unregister failed: user is not registered.\n");
        logged_in = 0;
        current_uid[0] = '\0';
        current_pass[0] = '\0';
    } else if (strcmp(status, "WRP") == 0) {
        printf("Unregister failed: wrong password.\n");
    } else if (strcmp(status, "ERR") == 0) {
        printf("Unregister failed: server reported protocol error.\n");
    } else {
        printf("Unregister failed: unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}

void handle_logout_command(void)
{
    char req[64];
    char resp[RESP_MAX];
    int n;
    char tag[8], status[8];

    if (!logged_in) {
        printf("No user is currently logged in.\n");
        return;
    }

    /* Send UDP logout request to server */
    snprintf(req, sizeof(req), "LOU %s %s\n", current_uid, current_pass);

    n = send_udp_request(es_ip, es_port, req, resp, sizeof(resp));
    if (n < 0) {
        perror("send_udp_request (logout)");
        return;
    }

    if (sscanf(resp, "%7s %7s", tag, status) != 2 || strcmp(tag, "RLO") != 0) {
        printf("Protocol error on logout reply: %s\n", resp);
        return;
    }

    /* Handle server response */
    if (strcmp(status, "OK") == 0) {
        printf("Logout successful. User %s logged out.\n", current_uid);
        logged_in = 0;
        current_uid[0] = '\0';
        current_pass[0] = '\0';
    } else if (strcmp(status, "NOK") == 0) {
        printf("Logout failed: user not logged in on server.\n");
        logged_in = 0;
        current_uid[0] = '\0';
        current_pass[0] = '\0';
    } else if (strcmp(status, "UNR") == 0) {
        printf("Logout failed: user is not registered.\n");
        logged_in = 0;
        current_uid[0] = '\0';
        current_pass[0] = '\0';
    } else if (strcmp(status, "WRP") == 0) {
        printf("Logout failed: wrong password.\n");
    } else if (strcmp(status, "ERR") == 0) {
        printf("Logout failed: server reported protocol error.\n");
    } else {
        printf("Logout failed: unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}

int handle_exit_command(void)
{
    if (logged_in) {
        printf("Logging out user %s before exit...\n", current_uid);
        logged_in = 0;
        memset(current_uid, 0, sizeof(current_uid));
        memset(current_pass, 0, sizeof(current_pass));
    }
    printf("Exiting application...\n");
    return RC_OK;
}

void handle_create_command(const char *command_line)
{
    char name[32], event_fname[64], event_date[32], event_time[16], num_str[16];
    int num_attendees;
    char req_header[512];
    char resp[RESP_MAX];
    int fd;
    ssize_t n;
    char tag[8], status[8], eid[8];

    if (!logged_in) {
        printf("You must be logged in to create an event.\n");
        return;
    }

    /* Parse command line arguments */
    if (sscanf(command_line, "create %31s %63s %31s %15s %15s", 
               name, event_fname, event_date, event_time, num_str) != 5) {
        printf("Usage: create name event_fname event_date event_time num_attendees\n");
        return;
    }

    /* Validate all input parameters */
    if (!valid_event_name(name)) {
        printf("Invalid event name. Maximum 10 alphanumeric characters.\n");
        return;
    }

    if (!valid_filename(event_fname)) {
        printf("Invalid filename. Max 24 chars, alphanumeric with '-', '_', '.' and 3-letter extension.\n");
        return;
    }

    if (!valid_event_date(event_date)) {
        printf("Invalid date format. Use dd-mm-yyyy.\n");
        return;
    }

    /* Validate time format (hh:mm) */
    int hour, minute;
    if (strlen(event_time) != 5 || event_time[2] != ':' ||
        sscanf(event_time, "%d:%d", &hour, &minute) != 2 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        printf("Invalid time format. Use hh:mm (e.g., 14:30).\n");
        return;
    }

    /* Check if event is in the future */
    int day, month, year;
    sscanf(event_date, "%d-%d-%d", &day, &month, &year);
    
    time_t now = time(NULL);
    struct tm event_tm = {0};
    event_tm.tm_year = year - 1900;
    event_tm.tm_mon = month - 1;
    event_tm.tm_mday = day;
    event_tm.tm_hour = hour;
    event_tm.tm_min = minute;
    event_tm.tm_sec = 0;
    event_tm.tm_isdst = -1;
    
    time_t event_time_t = mktime(&event_tm);
    if (event_time_t <= now) {
        printf("Event date and time must be in the future.\n");
        return;
    }

    num_attendees = atoi(num_str);
    if (num_attendees < 10 || num_attendees > 999) {
        printf("Invalid attendance size. Must be between 10 and 999.\n");
        return;
    }

    /* Read event file from disk */
    FILE *fp = fopen(event_fname, "rb");
    if (!fp) {
        printf("Failed to open file '%s'.\n", event_fname);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (fsize <= 0 || fsize > 10L * 1024L * 1024L) {
        printf("Invalid file size. Must be between 1 and 10MB.\n");
        fclose(fp);
        return;
    }

    char *fdata = malloc(fsize);
    if (!fdata) {
        printf("Memory allocation failed.\n");
        fclose(fp);
        return;
    }

    if (fread(fdata, 1, fsize, fp) != (size_t)fsize) {
        printf("Failed to read file '%s'.\n", event_fname);
        free(fdata);
        fclose(fp);
        return;
    }
    fclose(fp);

    /* Establish TCP connection to server */
    fd = connect_tcp(es_ip, es_port);
    if (fd < 0) {
        printf("Failed to connect to server.\n");
        free(fdata);
        return;
    }

    /* Build and send request with file data */
    char full_datetime[64];
    snprintf(full_datetime, sizeof(full_datetime), "%s %s", event_date, event_time);
    
    snprintf(req_header, sizeof(req_header), "CRE %s %s %s %s %d %s %ld ",
             current_uid, current_pass, name, full_datetime, num_attendees, event_fname, fsize);

    if (write_all(fd, req_header, strlen(req_header)) < 0) {
        printf("Failed to send request header.\n");
        free(fdata);
        close(fd);
        return;
    }

    if (write_all(fd, fdata, fsize) < 0) {
        printf("Failed to send file data.\n");
        free(fdata);
        close(fd);
        return;
    }
    
    if (write_all(fd, "\n", 1) < 0) {
        printf("Failed to send final newline.\n");
        free(fdata);
        close(fd);
        return;
    }
    
    free(fdata);

    /* Read response line */
    char *ptr = resp;
    int nread = 0;
    memset(resp, 0, sizeof(resp));
    while (nread < (int)sizeof(resp) - 1) {
        n = read(fd, ptr, 1);
        if (n <= 0) break;
        if (*ptr == '\n') { 
            ptr++; 
            nread++;
            break; 
        }
        ptr++;
        nread++;
    }
    *ptr = '\0';
    close(fd);

    /* Initialize eid to empty in case OK doesn't include it */
    eid[0] = '\0';
    
    int parsed = sscanf(resp, "%7s %7s %7s", tag, status, eid);
    if (parsed < 2 || strcmp(tag, "RCE") != 0) {
        printf("Protocol error on create reply: %s\n", resp);
        return;
    }

    if (strcmp(status, "OK") == 0) {
        if (parsed >= 3 && strlen(eid) > 0) {
            printf("Event created successfully. EID: %s\n", eid);
        } else {
            printf("Event created successfully.\n");
        }
    } else if (strcmp(status, "NOK") == 0) {
        printf("Event creation failed.\n");
    } else if (strcmp(status, "NLG") == 0) {
        printf("Event creation failed: user not logged in.\n");
        clear_local_login();
    } else if (strcmp(status, "WRP") == 0) {
        printf("Event creation failed: wrong password.\n");
    } else if (strcmp(status, "ERR") == 0) {
        printf("Event creation failed: server reported protocol error.\n");
    } else {
        printf("Event creation failed: unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}

void handle_close_command(const char *command_line)
{
    char eid_str[32];
    int eid_int;
    char eid_fmt[8];
    char req[64];
    char resp[BUF_SIZE];
    int n;
    char tag[8], status[8];

    if (!logged_in) {
        printf("You must be logged in to close an event.\n");
        return;
    }

    /* Parse and validate event ID */
    if (sscanf(command_line, "close %31s", eid_str) != 1) {
        printf("Usage: close EID\n");
        return;
    }

    eid_int = atoi(eid_str);
    if (!valid_eid_int(eid_int)) {
        printf("Invalid EID. It must be an integer between 1 and 999.\n");
        return;
    }

    /* Send TCP close request to server */
    snprintf(eid_fmt, sizeof(eid_fmt), "%03d", eid_int);
    snprintf(req, sizeof(req), "CLS %s %s %s\n", current_uid, current_pass, eid_fmt);

    n = send_tcp_request_line(es_ip, es_port, req, resp, sizeof(resp));
    if (n < 0) {
        perror("send_tcp_request_line (close)");
        return;
    }

    if (sscanf(resp, "%7s %7s", tag, status) != 2 || strcmp(tag, "RCL") != 0) {
        printf("Protocol error on close reply: %s\n", resp);
        return;
    }

    /* Handle server response */
    if (strcmp(status, "OK") == 0) {
        printf("Event %s successfully closed.\n", eid_fmt);
    } else if (strcmp(status, "NOK") == 0) {
        printf("Close failed: unknown user or wrong password.\n");
    } else if (strcmp(status, "NLG") == 0) {
        printf("Close failed: user not logged in.\n");
        clear_local_login();
    } else if (strcmp(status, "NOE") == 0) {
        printf("Close failed: event %s does not exist.\n", eid_fmt);
    } else if (strcmp(status, "EOW") == 0) {
        printf("Close failed: you are not the owner of event %s.\n", eid_fmt);
    } else if (strcmp(status, "SLD") == 0) {
        printf("Close failed: event %s is already sold out.\n", eid_fmt);
    } else if (strcmp(status, "PST") == 0) {
        printf("Close failed: event %s is already in the past.\n", eid_fmt);
    } else if (strcmp(status, "CLO") == 0) {
        printf("Close failed: event %s was already closed.\n", eid_fmt);
    } else if (strcmp(status, "ERR") == 0) {
        printf("Close failed: server reported protocol error.\n");
    } else {
        printf("Close failed: unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}

/**
 * state_to_str - Convert numeric event state to human-readable string
 * @s: State number as string ("0", "1", "2", "3")
 */
static const char *state_to_str(const char *s)
{
    if (strcmp(s, "0") == 0) return "past";
    if (strcmp(s, "1") == 0) return "future (seats available)";
    if (strcmp(s, "2") == 0) return "future (sold out)";
    if (strcmp(s, "3") == 0) return "closed by owner";
    return "unknown";
}

void handle_show_command(const char *command_line)
{
    char eid_str[32];
    int eid_int;
    char eid_fmt[8];
    char req[64];
    char header[BUF_SIZE];
    int fd;
    ssize_t n;

    /* Parse and validate event ID */
    if (sscanf(command_line, "show %31s", eid_str) != 1) {
        printf("Usage: show EID\n");
        return;
    }

    eid_int = atoi(eid_str);
    if (!valid_eid_int(eid_int)) {
        printf("Invalid EID. It must be an integer between 1 and 999.\n");
        return;
    }

    /* Connect and send TCP request to server */
    snprintf(eid_fmt, sizeof(eid_fmt), "%03d", eid_int);
    snprintf(req, sizeof(req), "SED %s\n", eid_fmt);

    fd = connect_tcp(es_ip, es_port);
    if (fd < 0) {
        printf("Failed to connect to server.\n");
        return;
    }

    if (write_all(fd, req, strlen(req)) < 0) {
        printf("Failed to send show request.\n");
        close(fd);
        return;
    }

    /* Read response - counting spaces to find where file data begins */
    memset(header, 0, sizeof(header));

    char *ptr = header;
    int nread = 0;
    int space_count = 0;
    int header_complete = 0;
    
    while (nread < (int)sizeof(header) - 1) {
        n = read(fd, ptr, 1);
        if (n <= 0) break;
        
        if (*ptr == '\n') {
            *ptr = '\0';
            header_complete = 1;
            break;
        }
        
        if (*ptr == ' ') {
            space_count++;
            /* Stop reading header after all fields are parsed */
            if (space_count == 10) {
                *ptr = '\0';
                header_complete = 1;
                break;
            }
        }
        
        ptr++;
        nread++;
    }
    
    if (!header_complete) {
        *ptr = '\0';
    }

    char tag[8] = {0}, status[8] = {0}, owner_uid[16] = {0}, name[NAME_MAX_LEN] = {0};
    char event_date[16] = {0}, event_time[16] = {0}, fname[EVENT_FILE_MAX_LEN] = {0};
    int attendance_size = 0, seats_reserved = 0;
    long fsize = 0;

    /* Parse: RSE status UID name dd-mm-yyyy hh:mm attendance_size seats_reserved Fname Fsize */
    int parsed = sscanf(header, "%7s %7s %15s %127s %15s %15s %d %d %255s %ld",
                        tag, status, owner_uid, name, event_date, event_time,
                        &attendance_size, &seats_reserved, fname, &fsize);

    if (parsed < 2 || strcmp(tag, "RSE") != 0) {
        printf("Protocol error on show reply: %s\n", header);
        close(fd);
        return;
    }

    if (strcmp(status, "OK") != 0) {
        if (strcmp(status, "NOK") == 0) {
            printf("Event not found or no file available.\n");
        } else if (strcmp(status, "ERR") == 0) {
            printf("Server reported protocol error.\n");
        } else {
            printf("Show failed: unknown status '%s'. Raw reply: %s\n", status, header);
        }
        close(fd);
        return;
    }

    /* For OK we expect full header fields (10 fields) */
    if (parsed < 10) {
        printf("Protocol error on show reply: incomplete header (parsed %d fields).\n", parsed);
        close(fd);
        return;
    }

    if (fsize <= 0 || fsize > 10L * 1024L * 1024L) {
        printf("Invalid file size reported: %ld bytes.\n", fsize);
        close(fd);
        return;
    }

    /* Build full path for the output file */
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "downloads/%s", fname);

    FILE *out = fopen(filepath, "wb");
    if (!out) {
        printf("Failed to create file '%s'.\n", filepath);
        close(fd);
        return;
    }

    long remaining = fsize;
    char buf[4096];
    while (remaining > 0) {
        ssize_t to_read = (remaining < (long)sizeof(buf)) ? remaining : (long)sizeof(buf);
        n = read(fd, buf, to_read);
        if (n <= 0) {
            printf("Error reading file data from server (remaining: %ld).\n", remaining);
            fclose(out);
            close(fd);
            return;
        }
        if (fwrite(buf, 1, n, out) != (size_t)n) {
            printf("Error writing to file '%s'.\n", fname);
            fclose(out);
            close(fd);
            return;
        }
        remaining -= n;
    }

    fclose(out);
    close(fd);

    int available = attendance_size - seats_reserved;
    if (available < 0) available = 0;

    printf("Event Details (EID %s):\n", eid_fmt);
    printf("  Owner UID: %s\n", owner_uid);
    printf("  Name: %s\n", name);
    printf("  Date: %s %s\n", event_date, event_time);
    printf("  Total Seats: %d\n", attendance_size);
    printf("  Reserved Seats: %d\n", seats_reserved);
    printf("  Available Seats: %d\n", available);
    printf("  File saved: %s (Size: %ld bytes)\n", filepath, fsize);
}

void handle_myevents_command(void)
{
    char req[64];
    char resp[UDP_RESP_MAX];
    int n;
    char tag[8], status[8];
    int offset = 0;

    if (!logged_in) {
        printf("You must be logged in to list your events.\n");
        return;
    }

    /* Send UDP request to list user's created events */
    snprintf(req, sizeof(req), "LME %s %s\n", current_uid, current_pass);

    n = send_udp_request(es_ip, es_port, req, resp, sizeof(resp));
    if (n < 0) {
        perror("send_udp_request (myevents)");
        return;
    }

    if (sscanf(resp, "%7s %7s %n", tag, status, &offset) < 2 || strcmp(tag, "RME") != 0) {
        printf("Protocol error on myevents reply: %s\n", resp);
        return;
    }

    /* Parse and display server response */
    if (strcmp(status, "NOK") == 0) {
        printf("You have not created any events.\n");
    } else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in.\n");
        clear_local_login();
    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password.\n");
    } else if (strcmp(status, "OK") == 0) {
        printf("Your Created Events:\n");
        char *p = resp + offset;
        char eid[8];
        int state, consumed;

        if (*p == '\0' || *p == '\n') {
            printf("  (no events created yet)\n");
            return;
        }

        while (sscanf(p, "%7s %d %n", eid, &state, &consumed) == 2) {
            char state_str[4];
            snprintf(state_str, sizeof(state_str), "%d", state);
            printf("  - EID %s | State: %s\n", eid, state_to_str(state_str));
            p += consumed;
        }
    } else if (strcmp(status, "ERR") == 0) {
        printf("Server reported protocol error.\n");
    } else {
        printf("Unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}

void handle_myreservations_command(void)
{
    char req[64];
    char resp[UDP_RESP_MAX];
    int n;
    char tag[8], status[8];
    int offset = 0;

    if (!logged_in) {
        printf("You must be logged in to list your reservations.\n");
        return;
    }

    /* Send UDP request to list user's reservations (max 50) */
    snprintf(req, sizeof(req), "LMR %s %s\n", current_uid, current_pass);

    n = send_udp_request(es_ip, es_port, req, resp, sizeof(resp));
    if (n < 0) {
        perror("send_udp_request (myreservations)");
        return;
    }

    if (sscanf(resp, "%7s %7s %n", tag, status, &offset) < 2 || strcmp(tag, "RMR") != 0) {
        printf("Protocol error on myreservations reply: %s\n", resp);
        return;
    }

    /* Parse and display server response */
    if (strcmp(status, "NOK") == 0) {
        printf("You have not made any reservations.\n");
    } else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in.\n");
        clear_local_login();
    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password.\n");
    } else if (strcmp(status, "OK") == 0) {
        printf("Your Reservations:\n");
        char *p = resp + offset;
        char eid[8], date[16], time_str[16];
        int value, consumed;

        if (*p == '\0' || *p == '\n') {
            printf("  (no reservations found)\n");
            return;
        }

        /* Format: EID dd-mm-yyyy hh:mm:ss value */
        while (sscanf(p, "%7s %15s %15s %d %n", eid, date, time_str, &value, &consumed) == 4) {
            printf("  - EID %s | Date: %s %s | Seats Reserved: %d\n", eid, date, time_str, value);
            p += consumed;
        }
    } else if (strcmp(status, "ERR") == 0) {
        printf("Server reported protocol error.\n");
    } else {
        printf("Unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}

void handle_list_command(void)
{
    char req[8] = "LST\n";
    char tag[8], status[8];
    int offset = 0;

    /* Connect to server and stream list of all events */
    int fd = connect_tcp(es_ip, es_port);
    if (fd < 0) {
        perror("connect_tcp (list)");
        return;
    }
    if (write_all(fd, req, strlen(req)) < 0) {
        perror("write_all (list)");
        close(fd);
        return;
    }

    /* Read response in chunks */
    size_t cap = 4096;
    size_t len = 0;
    char *resp = malloc(cap);
    if (!resp) {
        printf("Memory allocation failed.\n");
        close(fd);
        return;
    }

    for (;;) {
        char buf[1024];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n < 0) {
            perror("read (list)");
            free(resp);
            close(fd);
            return;
        }
        if (n == 0) break; /* connection closed by server */

        if (len + (size_t)n + 1 > cap) {
            size_t new_cap = cap * 2;
            while (new_cap < len + (size_t)n + 1) new_cap *= 2;
            char *tmp = realloc(resp, new_cap);
            if (!tmp) {
                printf("Memory allocation failed.\n");
                free(resp);
                close(fd);
                return;
            }
            resp = tmp;
            cap = new_cap;
        }

        memcpy(resp + len, buf, (size_t)n);
        len += (size_t)n;
    }
    close(fd);
    resp[len] = '\0';

    if (sscanf(resp, "%7s %7s %n", tag, status, &offset) < 2 || strcmp(tag, "RLS") != 0) {
        printf("Protocol error on list reply: %s\n", resp);
        free(resp);
        return;
    }

    if (strcmp(status, "NOK") == 0) {
        printf("No events have been created yet.\n");
        free(resp);
        return;
    } else if (strcmp(status, "ERR") == 0) {
        printf("List failed: server reported protocol error.\n");
        free(resp);
        return;
    } else if (strcmp(status, "OK") != 0) {
        printf("List failed: unknown status '%s'. Raw reply: %s\n", status, resp);
        free(resp);
        return;
    }

    char *p = resp + offset;
    char eid[8], name[NAME_MAX_LEN], state[8], date[16], time_str[16];
    int consumed;

    if (*p == '\n' || *p == '\0') {
        printf("No events in list.\n");
        free(resp);
        return;
    }
    printf("%s", p);
    printf("Events:\n");
    printf("  %-5s | %-10s | %-24s | %-16s\n", "EID", "Name", "State", "Date/Time");
    printf("  ------+------------+--------------------------+------------------\n");
    while (sscanf(p, "%7s %127s %7s %15s %15s %n",
                  eid, name, state, date, time_str, &consumed) == 5) {
        printf("  %-5s | %-10s | %-24s | %s %s\n",
               eid, name, state_to_str(state), date, time_str);
        p += consumed;
    }

    free(resp);
}

void handle_reserve_command(const char *command_line)
{
    char eid_str[32];
    char people_str[32];
    int eid_int, people_int;
    char eid_fmt[8];
    char req[128];
    char resp[BUF_SIZE];
    int n;
    char tag[8], status[8];
    int offset = 0;

    if (!logged_in) {
        printf("You must be logged in to reserve seats.\n");
        return;
    }

    /* Parse and validate input */
    if (sscanf(command_line, "reserve %31s %31s", eid_str, people_str) != 2) {
        printf("Usage: reserve EID value\n");
        return;
    }

    eid_int = atoi(eid_str);
    people_int = atoi(people_str);

    if (!valid_eid_int(eid_int)) {
        printf("Invalid EID. It must be an integer between 1 and 999.\n");
        return;
    }
    if (!valid_people_int(people_int)) {
        printf("Invalid value. It must be between 1 and 999.\n");
        return;
    }

    /* Send TCP reservation request to server */
    snprintf(eid_fmt, sizeof(eid_fmt), "%03d", eid_int);
    snprintf(req, sizeof(req), "RID %s %s %s %d\n",
             current_uid, current_pass, eid_fmt, people_int);

    n = send_tcp_request_line(es_ip, es_port, req, resp, sizeof(resp));
    if (n < 0) {
        perror("send_tcp_request_line (reserve)");
        return;
    }

    if (sscanf(resp, "%7s %7s %n", tag, status, &offset) < 2 || strcmp(tag, "RRI") != 0) {
        printf("Protocol error on reserve reply: %s\n", resp);
        return;
    }

    int remaining = -1;
    char *p = resp + offset;

    if (strcmp(status, "REJ") == 0) {
        /* expect n_seats after status */
        if (sscanf(p, "%d", &remaining) == 1) {
            printf("Reservation rejected: only %d seats remaining.\n", remaining);
        } else {
            printf("Reservation rejected: request exceeds remaining seats.\n");
        }
        return;
    }

    if (strcmp(status, "ACC") == 0) {
        printf("Reservation accepted: %d seat(s) reserved for event %s.\n",
               people_int, eid_fmt);
    } else if (strcmp(status, "NOK") == 0) {
        printf("Reservation failed: event %s is not active.\n", eid_fmt);
    } else if (strcmp(status, "NLG") == 0) {
        printf("Reservation failed: user not logged in.\n");
        clear_local_login();
    } else if (strcmp(status, "CLS") == 0) {
        printf("Reservation failed: event %s is closed.\n", eid_fmt);
    } else if (strcmp(status, "SLD") == 0) {
        printf("Reservation failed: event %s is sold out.\n", eid_fmt);
    } else if (strcmp(status, "PST") == 0) {
        printf("Reservation failed: event %s date has already passed.\n", eid_fmt);
    } else if (strcmp(status, "WRP") == 0) {
        printf("Reservation failed: wrong password.\n");
    } else if (strcmp(status, "ERR") == 0) {
        printf("Reservation failed: server reported protocol error.\n");
    } else {
        printf("Reservation failed: unknown status '%s'. Raw reply: %s\n", status, resp);
    }
}
