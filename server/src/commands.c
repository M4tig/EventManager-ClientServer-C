#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#include "constants.h"
#include "protocol.h"
#include "commands.h"
#include "database.h"

static int handle_login(const char *buffer, char *response, size_t resp_size) {
    char uid[UID_LEN + 2];
    char password[PASS_LEN + 2];
    char stored_pass[PASS_LEN + 2];
    
    /* Parse: LIN UID password\n */
    int parsed = sscanf(buffer, "LIN %7s %9s", uid, password);
    
    /* Validate parsing */
    if (parsed != 2) {
        snprintf(response, resp_size, "RLI ERR\n");
        return 1;
    }
    
    /* Validate UID format: exactly 6 digits */
    if (!valid_uid(uid)) {
        snprintf(response, resp_size, "RLI ERR\n");
        return 1;
    }
    
    /* Validate password format: exactly 8 alphanumeric */
    if (!valid_password(password)) {
        snprintf(response, resp_size, "RLI ERR\n");
        return 1;
    }
    
    if (user_registered(uid)) {
        /* verify password */
        if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
            snprintf(response, resp_size, "RLI ERR\n");
            return 1;
        }  
        if (strcmp(password, stored_pass) == 0) {
            create_login_file(uid);
            snprintf(response, resp_size, "RLI OK\n");
        } else {
            snprintf(response, resp_size, "RLI NOK\n");
        }
    } else {
        /* register new user */
        if (!create_user_dir(uid)) {
            snprintf(response, resp_size, "RLI ERR\n");
            return 1;
        }
        
        if (!create_password_file(uid, password)) {
            snprintf(response, resp_size, "RLI ERR\n");
            return 1;
        }
        
        create_login_file(uid);
        snprintf(response, resp_size, "RLI REG\n");
    }
    return 0;
}


static int handle_logout(const char *buffer, char *response, size_t resp_size) {
    char uid[UID_LEN + 2];
    char password[PASS_LEN + 2];
    char stored_pass[PASS_LEN + 2];

    // Parse: LOU UID password\n
    int parsed = sscanf(buffer, "LOU %7s %9s", uid, password);
    if (parsed != 2) {
        snprintf(response, resp_size, "RLO ERR\n");
        return 1;
    }
    if (!valid_uid(uid)) {
        snprintf(response, resp_size, "RLO ERR\n");
        return 1;
    }
    if (!valid_password(password)) {
        snprintf(response, resp_size, "RLO ERR\n");
        return 1;
    }
    if (!user_exists(uid)) {
        snprintf(response, resp_size, "RLO UNR\n");
        return 1;
    }
    if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
        snprintf(response, resp_size, "RLO ERR\n");
        return 1;
    }
    if (strcmp(password, stored_pass) != 0) {
        snprintf(response, resp_size, "RLO WRP\n");
        return 1;
    }
    if (!user_is_logged_in(uid)) {
        snprintf(response, resp_size, "RLO NOK\n");
        return 1;
    }
    // Remove login file
    if (!remove_login_file(uid)) {
        snprintf(response, resp_size, "RLO ERR\n");
        return 1;
    }
    snprintf(response, resp_size, "RLO OK\n");
    return 0;
}

static void handle_changepass(const char *buffer, char *response, size_t resp_size) {
    char uid[UID_LEN + 2];
    char oldpw[PASS_LEN + 2];
    char newpw[PASS_LEN + 2];
    char stored_pass[PASS_LEN + 2];

    // Parse: CPS UID oldPassword newPassword\n
    int parsed = sscanf(buffer, "CPS %7s %9s %9s", uid, oldpw, newpw);
    if (parsed != 3) {
        snprintf(response, resp_size, "RCP ERR\n");
        return;
    }
    if (!valid_uid(uid)) {
        snprintf(response, resp_size, "RCP ERR\n");
        return;
    }
    if (!valid_password(oldpw) || !valid_password(newpw)) {
        snprintf(response, resp_size, "RCP ERR\n");
        return;
    }
    if (!user_exists(uid)) {
        snprintf(response, resp_size, "RCP NID\n");
        return;
    }
    if (!user_is_logged_in(uid)) {
        snprintf(response, resp_size, "RCP NLG\n");
        return;
    }
    if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
        snprintf(response, resp_size, "RCP ERR\n");
        return;
    }
    if (strcmp(oldpw, stored_pass) != 0) {
        snprintf(response, resp_size, "RCP NOK\n");
        return;
    }
    // Change password
    if (!create_password_file(uid, newpw)) {
        snprintf(response, resp_size, "RCP ERR\n");
        return;
    }
    snprintf(response, resp_size, "RCP OK\n");
}

static void handle_create(int fd, const char *buffer, ssize_t buffer_len) {
    char uid[UID_LEN + 2], password[PASS_LEN + 2], stored_pass[PASS_LEN + 2];
    char name[NAME_MAX_LEN + 2], date[DATE_LEN + 2], time_str[TIME_LEN + 2];
    char fname[FNAME_MAX_LEN + 2], filepath[512], response[64];
    int attendance;
    long fsize;

    // Parse: CRE UID password name dd-mm-yyyy hh:mm attendance Fname Fsize Fdata
    int parsed = sscanf(buffer, "CRE %7s %9s %11s %11s %6s %d %25s %ld ",
                        uid, password, name, date, time_str, &attendance, fname, &fsize);
    if (parsed != 8) {
        write_all(fd, "RCE ERR\n", 8);
        return;
    }

    // Validate inputs
    if (!valid_uid(uid) || !valid_password(password) ||
        !valid_event_name(name) || !valid_event_date(date) ||
        !valid_event_time(time_str) || !valid_attendance(attendance) ||
        !valid_filename(fname) || fsize <= 0 || fsize > MAX_FILE_SIZE) {
        write_all(fd, "RCE ERR\n", 8);
        return;
    }
    // Check event date is in the future
    int day, month, year, hour, minute;
    sscanf(date, "%d-%d-%d", &day, &month, &year);
    sscanf(time_str, "%d:%d", &hour, &minute);
    
    time_t now = time(NULL);
    struct tm event_tm = {0};
    event_tm.tm_year = year - 1900;
    event_tm.tm_mon = month - 1;
    event_tm.tm_mday = day;
    event_tm.tm_hour = hour;
    event_tm.tm_min = minute;
    event_tm.tm_isdst = -1;
    
    if (mktime(&event_tm) <= now) {
        write_all(fd, "RCE ERR\n", 8);
        return;
    }

    // Check user exists and is logged in
    if (!user_exists(uid)) {
        write_all(fd, "RCE NOK\n", 8);
        return;
    }
    if (!user_is_logged_in(uid)) {
        write_all(fd, "RCE NLG\n", 8);
        return;
    }

    // Verify password
    if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
        write_all(fd, "RCE ERR\n", 8);
        return;
    }
    if (strcmp(password, stored_pass) != 0) {
        write_all(fd, "RCE WRP\n", 8);
        return;
    }

    // Allocate EID and create event structure
    int eid = allocate_eid();
    if (eid == 0 || !create_event_dir(eid)) {
        write_all(fd, "RCE NOK\n", 8);
        return;
    }

    // Find where Fdata starts
    char fsize_str[32];
    snprintf(fsize_str, sizeof(fsize_str), "%ld ", fsize);
    const char *data_ptr = strstr(buffer, fsize_str);
    if (!data_ptr) {
        write_all(fd, "RCE ERR\n", 8);
        return;
    }
    data_ptr += strlen(fsize_str);
    long available = buffer_len - (data_ptr - buffer);

    // Write description file
    snprintf(filepath, sizeof(filepath), "EVENTS/%03d/DESCRIPTION/%s", eid, fname);
    FILE *fp = fopen(filepath, "wb");
    if (fp == NULL) {
        write_all(fd, "RCE NOK\n", 8);
        return;
    }

    long file_written = 0;
    char filebuf[4096];

    while (file_written < fsize) {
        ssize_t n;
        if (available > 0) {
            // Use data from initial buffer
            n = (available < (long)sizeof(filebuf)) ? available : (ssize_t)sizeof(filebuf);
            memcpy(filebuf, data_ptr, n);
            data_ptr += n;
            available -= n;
        } else {
            // Read from socket
            ssize_t to_read = (fsize - file_written < (long)sizeof(filebuf)) ? (fsize - file_written) : (ssize_t)sizeof(filebuf);
            n = read(fd, filebuf, to_read);
            if (n <= 0) {
                fclose(fp);
                write_all(fd, "RCE ERR\n", 8);
                return;
            }
        }
        size_t file_bytes = (file_written + n > fsize) ? (fsize - file_written) : n;
        if (fwrite(filebuf, 1, file_bytes, fp) != file_bytes) {
            fclose(fp);
            write_all(fd, "RCE ERR\n", 8);
            return;
        }
        file_written += file_bytes;
    }
    fclose(fp);

    // Write event metadata
    if (!create_event_start_file(eid, uid, name, fname, date, time_str, attendance) ||
        !create_event_res_file(eid) || !mark_user_created_event(uid, eid)) {
        write_all(fd, "RCE NOK\n", 8);
        return;
    }

    snprintf(response, sizeof(response), "RCE OK %03d\n", eid);
    write_all(fd, response, strlen(response));
}

static void handle_unregister(const char *buffer, char *response, size_t resp_size) {
    char uid[UID_LEN + 2];
    char password[PASS_LEN + 2];
    char stored_pass[PASS_LEN + 2];
    int parsed = sscanf(buffer, "UNR %7s %9s", uid, password);
    if (parsed != 2) {
        snprintf(response, resp_size, "RUR ERR\n");
        return;
    }
    if (!valid_uid(uid)) {
        snprintf(response, resp_size, "RUR ERR\n");
        return;
    }
    if (!valid_password(password)) {
        snprintf(response, resp_size, "RUR ERR\n");
        return;
    }

    if (!user_registered(uid)) {
        snprintf(response, resp_size, "RUR UNR\n");
        return;
    }
    if (!user_is_logged_in(uid)) {
        snprintf(response, resp_size, "RUR NOK\n");
        return;
    }
    if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
        snprintf(response, resp_size, "RUR ERR\n");
        return;
    }
    if (strcmp(password, stored_pass) != 0){
        snprintf(response, resp_size, "RUR WRP\n");
        return;
    }
    if (!remove_login_file(uid)) {
        snprintf(response, resp_size, "RUR ERR\n");
        return;
    }
    if (!remove_password_file(uid)) {
        snprintf(response, resp_size, "RUR ERR\n");
        return;
    }
    snprintf(response, resp_size, "RUR OK\n");
}

static void handle_myevents(const char *buffer, char *response, size_t resp_size) {
    char uid[UID_LEN + 2];
    char password[PASS_LEN + 2];
    char stored_pass[PASS_LEN + 2];
    int parsed = sscanf(buffer, "LME %7s %9s", uid, password);
    if (parsed != 2) {
        snprintf(response, resp_size, "RME ERR\n");
        return;
    }
    if (!valid_uid(uid)) {
        snprintf(response, resp_size, "RME ERR\n");
        return;
    }
    if (!valid_password(password)) {
        snprintf(response, resp_size, "RME ERR\n");
        return;
    }
    if (!user_exists(uid)) {
        snprintf(response, resp_size, "RME UNR\n");
        return;
    }
    if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
        snprintf(response, resp_size, "RME ERR\n");
        return;
    }
    if (strcmp(password, stored_pass) != 0) {
        snprintf(response, resp_size, "RME WRP\n");
        return;
    }
    if (!user_is_logged_in(uid)) {
        snprintf(response, resp_size, "RME NLG\n");
        return;
    }
    if (list_user_events(uid, response, resp_size) != 0) {
        snprintf(response, resp_size, "RME NOK\n");
        return;
    }
    // TO DO create payload with created events
}

static void handle_myreservations(const char *buffer, char *response, size_t resp_size) {
    char uid[UID_LEN + 2];
    char password[PASS_LEN + 2];
    char stored_pass[PASS_LEN + 2];
    int parsed = sscanf(buffer, "LMR %7s %9s", uid, password);
    if (parsed != 2) {
        snprintf(response, resp_size, "RMR ERR\n");
        return;
    }
    if (!valid_uid(uid)) {
        snprintf(response, resp_size, "RMR ERR\n");
        return;
    }
    if (!valid_password(password)) {
        snprintf(response, resp_size, "RMR ERR\n");
        return;
    }
    if (!user_exists(uid)) {
        snprintf(response, resp_size, "RMR UNR\n");
        return;
    }
    if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
        snprintf(response, resp_size, "RMR ERR\n");
        return;
    }
    if (strcmp(password, stored_pass) != 0) {
        snprintf(response, resp_size, "RMR WRP\n");
        return;
    }
    if (!user_is_logged_in(uid)) {
        snprintf(response, resp_size, "RMR NLG\n");
        return;
    }
    if (list_user_reservations(uid, response, resp_size) != 0) {
        snprintf(response, resp_size, "RMR NOK\n");
        return;
    }
}

static void handle_list(const char *buffer, char *response, size_t resp_size) {
    (void)buffer;
    list_all_events(response, resp_size);
}

static void handle_show(int fd, const char *buffer) {
    int eid; 
    int parsed = sscanf(buffer, "SED %d", &eid);
    if (parsed != 1) {
        write_all(fd, "RSE ERR\n", 8);
        return;
    }
    
    if (!event_exists(eid)) {
        write_all(fd, "RSE NOK\n", 8);
        return;
    }
    
    // Get event details
    char uid[UID_LEN + 1], event_name[NAME_MAX_LEN + 1];
    if (!get_event_uid(eid, uid) || !get_event_name(eid, event_name)) {
        write_all(fd, "RSE NOK\n", 8);
        return;
    }
    
    char event_date[DATE_LEN + 1], event_time[TIME_LEN + 1];
    if (!get_event_date_time(eid, event_date, event_time)) {
        write_all(fd, "RSE NOK\n", 8);
        return;
    }
    
    int attendance = get_event_max_attendance(eid);
    if (attendance < 0) {
        write_all(fd, "RSE NOK\n", 8);
        return;
    }
    
    int reserved = get_event_reservations(eid);
    if (reserved < 0) {
        write_all(fd, "RSE NOK\n", 8);
        return;
    }
    
    char fname[FNAME_MAX_LEN + 1];
    if (!get_event_desc_fname(eid, fname)) {
        write_all(fd, "RSE NOK\n", 8);
        return;
    }

    char filepath[512];
    snprintf(filepath, sizeof(filepath), "EVENTS/%03d/DESCRIPTION/%s", eid, fname);
    
    size_t fsize = 0;
    if (!get_file_size_bytes(filepath, &fsize)) {
        write_all(fd, "RSE NOK\n", 8);
        return;
    }

    // Write response header
    char header[512];
    int header_len = snprintf(header, sizeof(header),
                              "RSE OK %s %s %s %s %d %d %s %zu ",
                              uid, event_name, event_date, event_time,
                              attendance, reserved, fname, fsize);
    
    if (header_len < 0 || (size_t)header_len >= sizeof(header)) {
        write_all(fd, "RSE ERR\n", 8);
        return;
    }
    
    write_all(fd, header, header_len);
    
    // Open and stream the file
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        write_all(fd, "\n", 1);
        return;
    }
    
    char filebuf[4096];
    size_t total_sent = 0;
    
    while (total_sent < fsize) {
        size_t to_read = (fsize - total_sent < sizeof(filebuf)) ? (fsize - total_sent) : sizeof(filebuf);
        size_t bytes_read = fread(filebuf, 1, to_read, fp);
        
        if (bytes_read == 0) {
            fclose(fp);
            write_all(fd, "\n", 1);
            return;
        }
        
        write_all(fd, filebuf, bytes_read);
        total_sent += bytes_read;
    }
    
    fclose(fp);
    write_all(fd, "\n", 1);
}

static void handle_reserve(const char *buffer, char *response, size_t resp_size) {
    char uid[UID_LEN + 2];
    char password[PASS_LEN + 2];
    char stored_pass[PASS_LEN + 2];
    char eid_str[EID_LEN + 2];
    int eid, people;

    // Parse: RID UID password EID people\n
    int parsed = sscanf(buffer, "RID %7s %9s %4s %d", uid, password, eid_str, &people);
    if (parsed != 4) {
        snprintf(response, resp_size, "RRI ERR\n");
        return;
    }

    // Validate formats
    if (!valid_uid(uid) || !valid_password(password)) {
        snprintf(response, resp_size, "RRI ERR\n");
        return;
    }

    // Validate and parse EID
    if (strlen(eid_str) != 3 || !isdigit(eid_str[0]) || 
        !isdigit(eid_str[1]) || !isdigit(eid_str[2])) {
        snprintf(response, resp_size, "RRI ERR\n");
        return;
    }
    eid = atoi(eid_str);

    // Validate people
    if (people < 1 || people > 999) {
        snprintf(response, resp_size, "RRI ERR\n");
        return;
    }

    // Check if user exists
    if (!user_exists(uid)) {
        snprintf(response, resp_size, "RRI NOK\n");
        return;
    }

    // Check if user is logged in
    if (!user_is_logged_in(uid)) {
        snprintf(response, resp_size, "RRI NLG\n");
        return;
    }

    // Verify password
    if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
        snprintf(response, resp_size, "RRI ERR\n");
        return;
    }
    if (strcmp(password, stored_pass) != 0) {
        snprintf(response, resp_size, "RRI WRP\n");
        return;
    }

    // Check if event exists
    if (!event_exists(eid)) {
        snprintf(response, resp_size, "RRI NOK\n");
        return;
    }

    // Check if event is already closed
    if (event_is_closed(eid)) {
        snprintf(response, resp_size, "RRI CLS\n");
        return;
    }

    // Check if event date has passed
    if (event_date_passed(eid)) {
        // Close event automatically with event date
        close_event_with_event_date(eid);
        snprintf(response, resp_size, "RRI PST\n");
        return;
    }

    // Get current reservations and max attendance
    int current_res = get_event_reservations(eid);
    int max_attendance = get_event_max_attendance(eid);
    
    if (current_res < 0 || max_attendance < 0) {
        snprintf(response, resp_size, "RRI ERR\n");
        return;
    }

    // Check if already sold out
    if (current_res >= max_attendance) {
        snprintf(response, resp_size, "RRI SLD\n");
        return;
    }

    // Check if requested seats fit
    int available = max_attendance - current_res;
    if (people > available) {
        snprintf(response, resp_size, "RRI REJ %d\n", available);
        return;
    }

    // Add reservation
    if (!add_reservation(eid, uid, people)) {
        snprintf(response, resp_size, "RRI ERR\n");
        return;
    }

    snprintf(response, resp_size, "RRI ACC\n");
}

static void handle_close(const char *buffer, char *response, size_t resp_size) {
    char uid[UID_LEN + 2];
    char password[PASS_LEN + 2];
    char stored_pass[PASS_LEN + 2];
    char eid_str[EID_LEN + 2];
    int eid;

    // Parse: CLS UID password EID\n
    int parsed = sscanf(buffer, "CLS %7s %9s %4s", uid, password, eid_str);
    if (parsed != 3) {
        snprintf(response, resp_size, "RCL ERR\n");
        return;
    }

    // Validate UID and password format
    if (!valid_uid(uid) || !valid_password(password)) {
        snprintf(response, resp_size, "RCL ERR\n");
        return;
    }

    // Validate and parse EID
    if (strlen(eid_str) != 3 || !isdigit(eid_str[0]) || 
        !isdigit(eid_str[1]) || !isdigit(eid_str[2])) {
        snprintf(response, resp_size, "RCL ERR\n");
        return;
    }
    eid = atoi(eid_str);

    // Check if user exists
    if (!user_exists(uid)) {
        snprintf(response, resp_size, "RCL NOK\n");
        return;
    }

    // Verify password
    if (!read_user_password(uid, stored_pass, sizeof(stored_pass))) {
        snprintf(response, resp_size, "RCL ERR\n");
        return;
    }
    if (strcmp(password, stored_pass) != 0) {
        snprintf(response, resp_size, "RCL NOK\n");
        return;
    }

    // Check if user is logged in
    if (!user_is_logged_in(uid)) {
        snprintf(response, resp_size, "RCL NLG\n");
        return;
    }

    // Check if event exists
    if (!event_exists(eid)) {
        snprintf(response, resp_size, "RCL NOE\n");
        return;
    }

    // Check if user created this event
    if (!event_is_created_by(eid, uid)) {
        snprintf(response, resp_size, "RCL EOW\n");
        return;
    }

    // Check if event is already closed
    if (event_is_closed(eid)) {
        snprintf(response, resp_size, "RCL CLO\n");
        return;
    }

    // Check if event date has passed
    if (event_date_passed(eid)) {
        snprintf(response, resp_size, "RCL PST\n");
        return;
    }

    // Check if event is sold out
    if (event_is_sold_out(eid)) {
        snprintf(response, resp_size, "RCL SLD\n");
        return;
    }

    // Close the event
    if (!close_event_now(eid)) {
        snprintf(response, resp_size, "RCL ERR\n");
        return;
    }

    snprintf(response, resp_size, "RCL OK\n");
}



void handle_udp_request(int ufd) {
    char buffer[BUF_SIZE];
    char response[BUF_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    
    ssize_t n = recvfrom(ufd, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&client_addr, &addrlen);
    if (n == -1) {
        perror("recvfrom");
        return;
    }
    buffer[n] = '\0';
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
    get_client_info(&client_addr, client_ip, &client_port);
    
    /* Extract request type and UID */
    char req_type[8] = {0};
    char uid[16] = {0};
    sscanf(buffer, "%7s %15s", req_type, uid);
    
    verbose_print("UDP", client_ip, client_port, req_type, uid);
    
    /* Default error response */
    snprintf(response, sizeof(response), "ERR\n");

    if (strcmp(req_type, "LIN") == 0) {
        handle_login(buffer, response, sizeof(response));
    } 
    else if (strcmp(req_type, "LOU") == 0) {
        handle_logout(buffer, response, sizeof(response));
    }
    else if (strcmp(req_type, "UNR") == 0) {
        handle_unregister(buffer, response, sizeof(response));
    }
    else if (strcmp(req_type, "LME") == 0) {
        static char large_resp[MAX_BUF_SIZE];
        handle_myevents(buffer, large_resp, sizeof(large_resp));
        sendto(ufd, large_resp, strlen(large_resp), 0,
               (struct sockaddr *)&client_addr, addrlen);
        return;
    }
    else if (strcmp(req_type, "LMR") == 0) {
        static char large_resp[MAX_BUF_SIZE];
        handle_myreservations(buffer, large_resp, sizeof(large_resp));
        sendto(ufd, large_resp, strlen(large_resp), 0,
               (struct sockaddr *)&client_addr, addrlen);
        return;
    }
    
    sendto(ufd, response, strlen(response), 0,
           (struct sockaddr *)&client_addr, addrlen);
}

void handle_tcp_connection(int newfd, struct sockaddr_in *client_addr) {
    char buffer[BUF_SIZE];
    
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
    get_client_info(client_addr, client_ip, &client_port);
    
    ssize_t n = read(newfd, buffer, sizeof(buffer) - 1);
    if (n <= 0) {
        close(newfd);
        return;
    }
    buffer[n] = '\0';
    char req_type[8] = {0};
    char uid[16] = {0};
    sscanf(buffer, "%7s %15s", req_type, uid);
    
    if (strcmp(req_type, "LST") == 0 || strcmp(req_type, "SED") == 0) {
        uid[0] = '\0';
    }
    
    verbose_print("TCP", client_ip, client_port, req_type, uid);
    
    const char *response = "ERR\n";
    
    if (strcmp(req_type, "CPS") == 0) {
        static char respbuf[BUF_SIZE];
        handle_changepass(buffer, respbuf, sizeof(respbuf));
        write_all(newfd, respbuf, strlen(respbuf));
        close(newfd);
        return;
    }
    else if (strcmp(req_type, "CRE") == 0) {
        handle_create(newfd, buffer, n);
        close(newfd);
        return;
    }
    else if (strcmp(req_type, "CLS") == 0) {
        static char respbuf[BUF_SIZE];
        handle_close(buffer, respbuf, sizeof(respbuf));
        write_all(newfd, respbuf, strlen(respbuf));
        close(newfd);
        return;
    }
    else if (strcmp(req_type, "LST") == 0) {
        static char large_resp[LIST_RESPONSE_SIZE];
        handle_list(buffer, large_resp, sizeof(large_resp));
        write_all(newfd, large_resp, strlen(large_resp));
        close(newfd);
        return;
    }
    else if (strcmp(req_type, "SED") == 0) {
        handle_show(newfd, buffer);
        close(newfd);
        return;
    }
    else if (strcmp(req_type, "RID") == 0) {
        static char respbuf[BUF_SIZE];
        handle_reserve(buffer, respbuf, sizeof(respbuf));
        write_all(newfd, respbuf, strlen(respbuf));
        close(newfd);
        return;
    }
    
    write_all(newfd, response, strlen(response));
    close(newfd);
}