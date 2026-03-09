#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <stdarg.h>

#include "constants.h"
#include "database.h"
#include <time.h>

// Helper struct for reservations listing

typedef struct {
    int eid;
    int value;
    char date[DATE_LEN + 1];   // dd-mm-yyyy
    char timefull[9 + 1];      // hh:mm:ss
    time_t ts;
} ResEntry;

// Small helpers and file functions

static int appendf(char *buf, size_t bufsz, size_t *used, const char *fmt, ...) {
    if (!buf || !used || *used >= bufsz) return 0;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf + *used, bufsz - *used, fmt, ap);
    va_end(ap);

    if (n < 0) return 0;
    if ((size_t)n >= bufsz - *used) return 0;

    *used += (size_t)n;
    return 1;
}

static int cmp_eid_then_time(const void *a, const void *b) {
    const ResEntry *ra = a, *rb = b;
    if (ra->eid != rb->eid) return (ra->eid < rb->eid) ? -1 : 1;
    if (ra->ts  != rb->ts)  return (ra->ts  < rb->ts)  ? -1 : 1; // oldest first
    return 0;
}

static int check_file_existence(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
}

int get_file_size_bytes(const char *path, size_t *out_size) {
    if (!path || !out_size) return 0;
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    if (!S_ISREG(st.st_mode)) return 0;
    *out_size = (size_t)st.st_size;
    return 1;
}

int dir_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
}

int dir_is_empty(const char *path) {
    if (!dir_exists(path)) return 1;

    DIR *dir = opendir(path);
    if (!dir) return 1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return 1;
}

int create_user_dir(const char *uid) {
    if (!valid_uid(uid)) return 0;
    
    char path[64];
    
    /* Create USERS/(uid)/ */
    snprintf(path, sizeof(path), "USERS/%s", uid);
    if (mkdir(path, 0700) == -1 && !dir_exists(path)) return 0;
    
    /* Create USERS/(uid)/CREATED/ */
    snprintf(path, sizeof(path), "USERS/%s/CREATED", uid);
    if (mkdir(path, 0700) == -1 && !dir_exists(path)) {
        return 0;
    }
    
    /* Create USERS/(uid)/RESERVED/ */
    snprintf(path, sizeof(path), "USERS/%s/RESERVED", uid);
    if (mkdir(path, 0700) == -1 && !dir_exists(path)) {
        return 0;
    }
    
    return 1;
}

int create_event_dir(int eid) {
    if (eid < 1 || eid > 999) return 0;
    
    char path[PATH_MAX];
    
    /* Create EVENTS/EID */
    snprintf(path, sizeof(path), "EVENTS/%03d", eid);
    if (mkdir(path, 0700) == -1) return 0;
    
    /* Create EVENTS/EID/DESCRIPTION */
    snprintf(path, sizeof(path), "EVENTS/%03d/DESCRIPTION", eid);
    if (mkdir(path, 0700) == -1) {
        char eid_path[PATH_MAX];
        snprintf(eid_path, sizeof(eid_path), "EVENTS/%03d", eid);
        rmdir(eid_path);
        return 0;
    }
    
    /* Create EVENTS/EID/RESERVATIONS */
    snprintf(path, sizeof(path), "EVENTS/%03d/RESERVATIONS", eid);
    if (mkdir(path, 0700) == -1) {
        char desc_path[PATH_MAX], eid_path[PATH_MAX];
        snprintf(desc_path, sizeof(desc_path), "EVENTS/%03d/DESCRIPTION", eid);
        snprintf(eid_path, sizeof(eid_path), "EVENTS/%03d", eid);
        rmdir(desc_path);
        rmdir(eid_path);
        return 0;
    }
    
    return 1;
}

int create_login_file(const char *uid) {
    if (!valid_uid(uid)) return 0;
    
    char path[64];
    snprintf(path, sizeof(path), "USERS/%s/%s_login.txt", uid, uid);
    
    FILE *fp = fopen(path, "w");
    if (fp == NULL) return 0;
    
    fprintf(fp, "Logged in\n");
    fclose(fp);
    return 1;
}

int remove_login_file(const char *uid) {
    if (!valid_uid(uid)) return 0;
    
    char path[64];
    snprintf(path, sizeof(path), "USERS/%s/%s_login.txt", uid, uid);
    unlink(path);
    return 1;
}

int create_password_file(const char *uid, const char *password) {
    if (!valid_uid(uid) || !valid_password(password)) return 0;
    
    char path[64];
    snprintf(path, sizeof(path), "USERS/%s/%s_pass.txt", uid, uid);
    
    FILE *fp = fopen(path, "w");
    if (fp == NULL) return 0;
    
    fprintf(fp, "%s\n", password);
    fclose(fp);
    return 1;
}

int remove_password_file(const char *uid) {
    if (!valid_uid(uid)) return 0;
    
    char path[64];
    snprintf(path, sizeof(path), "USERS/%s/%s_pass.txt", uid, uid);
    unlink(path);
    return 1;
}

int create_event_start_file(int eid, const char *uid, const char *name, 
                      const char *fname, const char *date, 
                      const char *time, int attendance) {
    if (eid < 1 || eid > 999) return 0;
    
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    
    FILE *fp = fopen(path, "w");
    if (fp == NULL) return 0;
    
    /* Format: UID name fname date time attendance */
    fprintf(fp, "%s %s %s %s %s %d\n", uid, name, fname, date, time, attendance);
    fclose(fp);
    return 1;
}

int create_event_res_file(int eid) {
    if (eid < 1 || eid > 999) return 0;
    
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    
    FILE *fp = fopen(path, "w");
    if (fp == NULL) return 0;
    
    fprintf(fp, "0\n");
    fclose(fp);
    return 1;
}

int mark_user_created_event(const char *uid, int eid) {
    if (!valid_uid(uid) || eid < 1 || eid > 999) return 0;
    
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "USERS/%s/CREATED/%03d.txt", uid, eid);
    
    FILE *fp = fopen(path, "w");
    if (fp == NULL) return 0;
    
    fprintf(fp, "Event %03d\n", eid);
    fclose(fp);
    return 1;
}
//////////////////////////////////////////////////////////////////////////////

// Time related functions

time_t now_time(void) {
    return time(NULL);   // seconds since Unix epoch
}

int compare_time(time_t a, time_t b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int parse_date_time_to_time(const char *date,
                            const char *timestr,
                            time_t *out,
                            int has_seconds)
{
    int dd, mm, yyyy, HH, MIN, SEC = 0;

    if (!date || !timestr || !out) return 0;

    // date: dd-mm-yyyy
    if (sscanf(date, "%2d-%2d-%4d", &dd, &mm, &yyyy) != 3) return 0;

    // time: hh:mm  OR  hh:mm:ss
    if (has_seconds) {
        if (sscanf(timestr, "%2d:%2d:%2d", &HH, &MIN, &SEC) != 3) return 0;
    } else {
        if (sscanf(timestr, "%2d:%2d", &HH, &MIN) != 2) return 0;
        SEC = 0;
    }

    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_mday  = dd;
    t.tm_mon   = mm - 1;
    t.tm_year  = yyyy - 1900;
    t.tm_hour  = HH;
    t.tm_min   = MIN;
    t.tm_sec   = SEC;
    t.tm_isdst = -1;

    time_t ts = mktime(&t);
    if (ts == (time_t)-1) return 0;

    *out = ts;
    return 1;
}

int event_date_passed(int eid) {
    if (eid < 1 || eid > 999) return 0;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);

    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    char uid[UID_LEN + 1], name[64], desc[128], date[16], hhmm[8];
    int capacity;

    int n = fscanf(fp, "%6s %63s %127s %15s %7s %d",
                   uid, name, desc, date, hhmm, &capacity);
    fclose(fp);

    if (n != 6) return 0;

    time_t event_t;
    if (!parse_date_time_to_time(date, hhmm, &event_t, 0)) return 0;

    return (compare_time(event_t, now_time()) <= 0);
}

/////////////////////////////////////////////////////////////////////////////

// Getter functions

int get_event_reservations(int eid) {
    if (eid < 1 || eid > 999) return -1;
    
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    
    int current_res = 0;
    if (fscanf(fp, "%d", &current_res) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return current_res;
}

int get_event_desc_fname(int eid, char *buffer) {
    if (eid < 1 || eid > 999 || buffer == NULL) return 0;
    
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char uid[UID_LEN + 1], name[64], fname[128], date[16], hhmm[8];
    int attendance;
    int n = fscanf(fp, "%6s %63s %127s %15s %7s %d",
                   uid, name, fname, date, hhmm, &attendance);
    fclose(fp);
    if (n != 6) return 0;
    snprintf(buffer, FNAME_MAX_LEN + 1, "%s", fname);
    return 1;
}

int get_event_max_attendance(int eid) {
    if (eid < 1 || eid > 999) return -1;
    
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    FILE *fp = fopen(path, "r");
    if (!fp) return -1;
    char uid[UID_LEN + 1], name[64], fname[128], date[16], hhmm[8];
    int attendance;
    int n = fscanf(fp, "%6s %63s %127s %15s %7s %d",
                   uid, name, fname, date, hhmm, &attendance);
    fclose(fp);
    if (n != 6) return -1;
    return attendance;
}

int get_event_uid(int eid, char *buffer){

    if (eid < 1 || eid > 999 || buffer == NULL) return 0;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char uid[UID_LEN + 1], name[64], fname[128], date[16], hhmm[8];
    int attendance;
    int n = fscanf(fp, "%6s %63s %127s %15s %7s %d",
                   uid, name, fname, date, hhmm, &attendance);
    fclose(fp);
    if (n != 6) return 0;
    snprintf(buffer, UID_LEN + 1, "%s", uid);
    return 1;
}

int get_event_date_time(int eid, char *date_buffer, char *time_buffer){

    if (eid < 1 || eid > 999 || date_buffer == NULL || time_buffer == NULL) return 0;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);

    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    
    char uid[UID_LEN + 1];
    char name[NAME_MAX_LEN + 1];
    char fname[FNAME_MAX_LEN + 1];
    char date[DATE_LEN + 1];   // 11
    char hhmm[TIME_LEN + 1];   // 6
    int attendance;
    
    int n = fscanf(fp, "%6s %63s %127s %15s %7s %d",
                   uid, name, fname, date, hhmm, &attendance);
    fclose(fp);

    if (n != 6) return 0;

    snprintf(date_buffer, DATE_LEN + 1, "%s", date);
    snprintf(time_buffer, TIME_LEN + 1, "%s", hhmm);
    return 1;
}

int get_event_name(int eid, char *buffer){

    if (eid < 1 || eid > 999 || buffer == NULL) return 0;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);

    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    
    char uid[UID_LEN + 1];
    char name[NAME_MAX_LEN + 1];
    char fname[FNAME_MAX_LEN + 1];
    char date[DATE_LEN + 1];   // 11
    char hhmm[TIME_LEN + 1];   // 6
    int attendance;
    
    int n = fscanf(fp, "%6s %63s %127s %15s %7s %d",
                   uid, name, fname, date, hhmm, &attendance);
    fclose(fp);

    if (n != 6) return 0;

    snprintf(buffer, NAME_MAX_LEN + 1, "%s", name);
    return 1;
}

////////////////////////////////////////////////////////////////////////////

// Validation functions

int valid_uid(const char *uid) {
    if (strlen(uid) != UID_LEN) return 0;
    for (int i = 0; i < UID_LEN; i++)
        if (!isdigit((unsigned char)uid[i])) return 0;
    return 1;
}

int valid_password(const char *pw) {
    if (strlen(pw) != PASS_LEN) return 0;
    for (int i = 0; i < PASS_LEN; i++)
        if (!isalnum((unsigned char)pw[i])) return 0;
    return 1;
}

int valid_event_name(const char *name) {
    if (name == NULL) return 0;
    size_t len = strlen(name);
    if (len == 0 || len > NAME_MAX_LEN) return 0;
    for (size_t i = 0; i < len; i++) {
        if (!isalnum((unsigned char)name[i])) return 0;
    }
    return 1;
}

int valid_event_date(const char *date) {
    int day, month, year;
    int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (date == NULL || strlen(date) != DATE_LEN) return 0;
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

int valid_event_time(const char *time) {
    int hour, min;
    
    if (time == NULL || strlen(time) != TIME_LEN) return 0;
    if (time[2] != ':') return 0;
    if (sscanf(time, "%d:%d", &hour, &min) != 2) return 0;
    
    if (hour < 0 || hour > 23) return 0;
    if (min < 0 || min > 59) return 0;
    
    return 1;
}

int valid_attendance(int attendance) {
    return (attendance >= MIN_ATTENDANCE && attendance <= MAX_ATTENDANCE);
}

int valid_filename(const char *fname) {
    if (fname == NULL) return 0;
    int len = strlen(fname);
    if (len == 0 || len > FNAME_MAX_LEN) return 0;
    
    /* Check for valid extension (dot + 3 letters) */
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
    
    /* Check rest of filename is alphanumeric + '-', '_' */
    for (int i = 0; i < dot_pos; i++) {
        char c = fname[i];
        if (!isalnum((unsigned char)c) && c != '-' && c != '_') return 0;
    }
    
    return 1;
}

int server_has_events(){
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS");

    return !dir_is_empty(path);
}

////////////////////////////////////////////////////////////////////////////

// User functions

int user_registered(const char *uid) {
    if (!valid_uid(uid)) return 0;

    char path[PATH_MAX];

    int n = snprintf(path, sizeof(path), "USERS/%s/%s_pass.txt", uid, uid);
    if (n < 0 || (size_t)n >= sizeof(path)) return 0;

    return check_file_existence(path);
}

int user_exists(const char *uid) {
    char path[PATH_MAX];

    int n = snprintf(path, sizeof(path), "USERS/%s", uid);
    if (n < 0 || (size_t)n >= sizeof(path)) return 0;

    return dir_exists(path);
}

int user_is_logged_in(const char *uid) {
    if (!valid_uid(uid)) return 0;

    char path[PATH_MAX];

    int n = snprintf(path, sizeof(path), "USERS/%s/%s_login.txt", uid, uid);
    if (n < 0 || (size_t)n >= sizeof(path)) return 0;

    return check_file_existence(path);
}

int user_created_events(const char *uid) {
    if (!valid_uid(uid)) return 0;

    char path[PATH_MAX];
    int n = snprintf(path, sizeof(path), "USERS/%s/CREATED", uid);
    if (n < 0 || (size_t)n >= sizeof(path)) return 0;

    return !dir_is_empty(path);
}

int user_reserved_events(const char *uid) {
    if (!valid_uid(uid)) return 0;

    char path[PATH_MAX];
    int n = snprintf(path, sizeof(path), "USERS/%s/RESERVED", uid);
    if (n < 0 || (size_t)n >= sizeof(path)) return 0;

    return !dir_is_empty(path);
}

int read_user_password(const char *uid, char *password, size_t maxlen) {
    if (!valid_uid(uid) || password == NULL || maxlen < PASS_LEN + 1) return 0;
    
    char path[64];
    snprintf(path, sizeof(path), "USERS/%s/%s_pass.txt", uid, uid);
    
    FILE *fp = fopen(path, "r");
    if (fp == NULL) return 0;
    
    if (fgets(password, maxlen, fp) == NULL) {
        fclose(fp);
        return 0;
    }
    
    /* Remove trailing newline */
    password[strcspn(password, "\n\r")] = '\0';
    
    fclose(fp);
    return 1;
}
////////////////////////////////////////////////////////////////////////////

// Event state logic functions

int event_exists(int eid) {
    if (eid < 1 || eid > 999) return 0;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d", eid);
    return dir_exists(path);
}

int event_is_created_by(int eid, const char *uid) {
    if (eid < 1 || eid > 999 || !valid_uid(uid)) return 0;
    
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    
    char stored_uid[UID_LEN + 2];
    if (fscanf(fp, "%6s", stored_uid) != 1) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return strcmp(uid, stored_uid) == 0;
}

int event_is_closed(int eid) {
    if (eid < 1 || eid > 999) return 0;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/END.txt", eid);
    struct stat st;
    return (stat(path, &st) == 0);
}

int event_is_sold_out(int eid) {
    if (eid < 1 || eid > 999) return 0;

    char start_path[PATH_MAX];
    char res_path[PATH_MAX];
    snprintf(start_path, sizeof(start_path), "EVENTS/%03d/START_%03d.txt", eid, eid);
    snprintf(res_path, sizeof(res_path), "EVENTS/%03d/RES_%03d.txt", eid, eid);

    // Read max attendance from START file (single line)
    FILE *fp = fopen(start_path, "r");
    if (!fp) return 0;

    char uid[UID_LEN + 1];
    char name[64], fname[128], date[16], hhmm[8];
    int max_attendance = 0;

    int n = fscanf(fp, "%6s %63s %127s %15s %7s %d",
                   uid, name, fname, date, hhmm, &max_attendance);
    fclose(fp);

    if (n != 6) return 0;
    if (max_attendance < 0) return 0;

    // Read current reservations from RES file
    fp = fopen(res_path, "r");
    if (!fp) return 0;

    int current_res = 0;
    if (fscanf(fp, "%d", &current_res) != 1) {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    return (current_res >= max_attendance);
}

/////////////////////////////////////////////////////////////////////////

// Event mutation functions

int allocate_eid(void) {
    /* Find next available EID from 001 to 999 */
    for (int eid = 1; eid <= 999; eid++) {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "EVENTS/%03d", eid);
        if (!dir_exists(path)) {
            return eid;
        }
    }
    return 0;  /* No EID available */
}

int event_state(int eid){
    if(eid < 1 || eid > 999) return -1;
    if(event_is_closed(eid)){
        return 3; // Event is closed
    }
    if(event_date_passed(eid)){
        return 0; // Event date has passed
    }
    if(event_is_sold_out(eid)){
        return 2; // Event is sold out
    }
    if(!event_is_sold_out(eid)){
        return 1; // Event is available
    }

    return -1; // Unknown state
}

int add_reservation(int eid, const char *uid, int people) {
    if (eid < 1 || eid > 999 || !valid_uid(uid) || people < 1 || people > 999) return 0;
    
    // Get current reservation count
    int current_res = get_event_reservations(eid);
    if (current_res < 0) return 0;
    
    // Update RES file
    char res_path[PATH_MAX];
    snprintf(res_path, sizeof(res_path), "EVENTS/%03d/RES_%03d.txt", eid, eid);
    FILE *fp = fopen(res_path, "w");
    if (!fp) return 0;
    fprintf(fp, "%d\n", current_res + people);
    fclose(fp);
    
    // Get current date/time for reservation
    time_t now = now_time();
    struct tm *tm_info = localtime(&now);
    
    char res_datetime[32];
    int w = snprintf(res_datetime, sizeof(res_datetime),
                    "%02d-%02d-%04d %02d:%02d:%02d",
                    tm_info->tm_mday,
                    tm_info->tm_mon + 1,
                    tm_info->tm_year + 1900,
                    tm_info->tm_hour,
                    tm_info->tm_min,
                    tm_info->tm_sec);

    if (w < 0 || (size_t)w >= sizeof(res_datetime)) return 0;
    
    char filename[64];
    snprintf(filename, sizeof(filename), "R-%s-%04d-%02d-%02d %02d%02d%02d.txt",
            uid,
            tm_info->tm_year + 1900,
            tm_info->tm_mon + 1,
            tm_info->tm_mday,
            tm_info->tm_hour,
            tm_info->tm_min,
            tm_info->tm_sec);
    
    // Create reservation file content
    char content[128];
    snprintf(content, sizeof(content), "%s %d %s\n", uid, people, res_datetime);
    
    // Write to EVENTS/EID/RESERVATIONS/
    char event_res_path[PATH_MAX];
    snprintf(event_res_path, sizeof(event_res_path), "EVENTS/%03d/RESERVATIONS/%s", eid, filename);
    fp = fopen(event_res_path, "w");
    if (!fp) return 0;
    fprintf(fp, "%s", content);
    fclose(fp);
    
    // Write to USERS/UID/RESERVED/ (with EID added)
    char user_res_path[PATH_MAX];
    snprintf(user_res_path, sizeof(user_res_path), "USERS/%s/RESERVED/%s", uid, filename);
    fp = fopen(user_res_path, "w");
    if (!fp) return 0;
    fprintf(fp, "%s %d %s %03d\n", uid, people, res_datetime, eid);
    fclose(fp);
    
    return 1;
}

static int close_event_with_date(int eid, const char *close_date) {
    if (eid < 1 || eid > 999 || !close_date) return 0;
    
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/END.txt", eid);
    
    FILE *fp = fopen(path, "w");
    if (!fp) return 0;
    
    fprintf(fp, "%s\n", close_date);
    fclose(fp);
    return 1;
}

int close_event_now(int eid) {
    if (eid < 1 || eid > 999) return 0;
    
    time_t now = now_time();
    struct tm *tm_info = localtime(&now);
    char close_date[32];
    int w = snprintf(close_date, sizeof(close_date),
                     "%02d-%02d-%04d %02d:%02d:%02d",
                     tm_info->tm_mday,
                     tm_info->tm_mon + 1,
                     tm_info->tm_year + 1900,
                     tm_info->tm_hour,
                     tm_info->tm_min,
                     tm_info->tm_sec);

    if (w < 0 || (size_t)w >= sizeof(close_date)) return 0;
    
    return close_event_with_date(eid, close_date);
}

int close_event_with_event_date(int eid) {
    if (eid < 1 || eid > 999) return 0;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS/%03d/START_%03d.txt", eid, eid);

    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    char uid[UID_LEN + 1];
    char name[NAME_MAX_LEN + 1];
    char fname[FNAME_MAX_LEN + 1];
    char date[DATE_LEN + 1];      // dd-mm-yyyy
    char hhmm[TIME_LEN + 1];      // hh:mm
    int attendance = 0;

    int n = fscanf(fp, "%6s %63s %63s %10s %5s %d",
                   uid, name, fname, date, hhmm, &attendance);
    fclose(fp);

    if (n != 6) return 0;

    // END.txt wants: "dd-mm-yyyy hh:mm:ss"
    char close_date[32];
    snprintf(close_date, sizeof(close_date), "%s %s:00", date, hhmm);

    return close_event_with_date(eid, close_date);
}

////////////////////////////////////////////////////////////////////////////

// Commands

int list_user_events(const char *uid, char *response, size_t resp_size) {
    if (!valid_uid(uid)) return -1;

    char path[PATH_MAX];
    int n = snprintf(path, sizeof(path), "USERS/%s/CREATED", uid);
    if (n < 0 || (size_t)n >= sizeof(path)){
        snprintf(response, resp_size, "RME ERR\n");
        return -1;
    }

    if (!user_created_events(uid)) {
        snprintf(response, resp_size, "RME NOK\n");
        return 0;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        snprintf(response, resp_size, "RME ERR\n");
        return -1;
    }

    // Collect all EIDs first
    int eids[999];
    int count = 0;

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;

        // Expect "DDD.txt" where DDD are digits
        if (strlen(ent->d_name) != 7 ||
            !isdigit((unsigned char)ent->d_name[0]) ||
            !isdigit((unsigned char)ent->d_name[1]) ||
            !isdigit((unsigned char)ent->d_name[2]) ||
            strcmp(ent->d_name + 3, ".txt") != 0) {
            continue;
        }

        int eid = (ent->d_name[0]-'0')*100 + (ent->d_name[1]-'0')*10 + (ent->d_name[2]-'0');
        if (eid < 1 || eid > 999 || count >= 999) continue;

        eids[count++] = eid;
    }

    closedir(dir);

    // Sort EIDs in ascending order
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (eids[i] > eids[j]) {
                int temp = eids[i];
                eids[i] = eids[j];
                eids[j] = temp;
            }
        }
    }

    size_t used = 0;
    if (!appendf(response, resp_size, &used, "RME OK")) {
        snprintf(response, resp_size, "RME ERR\n");
        return -1;
    }

    // Build response with sorted EIDs
    for (int i = 0; i < count; i++) {
        int st = event_state(eids[i]);
        if (st < 0) continue;

        if (!appendf(response, resp_size, &used, " %03d %d", eids[i], st)) {
            snprintf(response, resp_size, "RME ERR\n");
            return -1;
        }
    }

    // End message
    if (!appendf(response, resp_size, &used, "\n")) {
        snprintf(response, resp_size, "RME ERR\n");
        return -1;
    }
    return 0;
}

int list_user_reservations(const char *uid, char *response, size_t resp_size) {
    char path[PATH_MAX];
    int n = snprintf(path, sizeof(path), "USERS/%s/RESERVED", uid);
    if (n < 0 || (size_t)n >= sizeof(path)) return -1;

    if(!user_reserved_events(uid)) {
        snprintf(response, resp_size, "RMR NOK\n");
        return 0;
    }

    struct dirent **namelist = NULL;
    int cnt = scandir(path, &namelist, NULL, alphasort);
    if (cnt <= 0) {
        snprintf(response, resp_size, "RMR NOK\n");
        return 0;
    }

    ResEntry sel[50];
    int sel_n = 0;

    // pick up to 50 most recent by filename order (newest at end)
    for (int i = cnt - 1; i >= 0 && sel_n < 50; i--) {
        const char *fname = namelist[i]->d_name;
        if (fname[0] == '.') continue;

        char fpath[PATH_MAX];
        int nn = snprintf(fpath, sizeof(fpath), "%s/%s", path, fname);
        if (nn < 0 || nn >= (int)sizeof(fpath)) {
            // Path would be too long (avoid truncation)
            continue;
        }
        FILE *fp = fopen(fpath, "r");
        if (!fp) continue;

        char f_uid[UID_LEN + 1];
        int eid = 0, value = 0;
        char date[DATE_LEN + 1];
        char timefull[16];

        // UID value date time EID
        int r = fscanf(fp, "%6s %d %10s %15s %d", f_uid, &value, date, timefull, &eid);
        fclose(fp);

        if (r != 5) continue;
        if (strcmp(f_uid, uid) != 0) continue;
        if (eid < 1 || eid > 999) continue;
        if (value < 1 || value > 999) continue;

        time_t ts;
        if (!parse_date_time_to_time(date, timefull, &ts, 1)) continue;

        sel[sel_n].eid = eid;
        sel[sel_n].value = value;
        strncpy(sel[sel_n].date, date, sizeof(sel[sel_n].date));
        sel[sel_n].date[DATE_LEN] = '\0';
        strncpy(sel[sel_n].timefull, timefull, sizeof(sel[sel_n].timefull));
        sel[sel_n].timefull[9] = '\0';
        sel[sel_n].ts = ts;

        sel_n++;
    }

    for (int i = 0; i < cnt; i++) free(namelist[i]);
    free(namelist);

    if (sel_n == 0) {
        snprintf(response, resp_size, "RMR NOK\n");
        return 0;
    }

    // NOW sort the selected 50 by EID then date/time
    qsort(sel, sel_n, sizeof(sel[0]), cmp_eid_then_time);

    // build response
    size_t used = 0;
    if (!appendf(response, resp_size, &used, "RMR OK")) {
        snprintf(response, resp_size, "RMR ERR\n");
        return -1;
    }

    for (int i = 0; i < sel_n; i++) {
        if (!appendf(response, resp_size, &used, " %03d %s %s %d",
                     sel[i].eid, sel[i].date, sel[i].timefull, sel[i].value)) {
            snprintf(response, resp_size, "RMR ERR\n");
            return -1;
        }
    }

    appendf(response, resp_size, &used, "\n");
    return 0;
}

int list_all_events(char *response, size_t resp_size) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "EVENTS");

    if (!server_has_events()) {
        snprintf(response, resp_size, "RLS NOK\n");
        return 0;
    }

    DIR *dir = opendir("EVENTS");
    if (!dir) {
        snprintf(response, resp_size, "RLS ERR\n");
        return -1;
    }

    size_t used = 0;

    if (!appendf(response, resp_size, &used, "RLS OK")) {
        closedir(dir);
        return -1;
    }

    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;

        if (strlen(e->d_name) != 3 ||
            !isdigit((unsigned char)e->d_name[0]) ||
            !isdigit((unsigned char)e->d_name[1]) ||
            !isdigit((unsigned char)e->d_name[2])) {
            continue;
        }

        int eid = atoi(e->d_name);

        char event_name[NAME_MAX_LEN + 1];
        if (!get_event_name(eid, event_name)) {
            closedir(dir);
            return -1;
        }
        char event_date[DATE_LEN + 1];
        char event_time[TIME_LEN + 1];
        if (!get_event_date_time(eid, event_date, event_time)) {
            closedir(dir);
            return -1;
        }

        int state = event_state(eid);
        if (state == -1) {
            closedir(dir);
            return -1;
        }

        if (!appendf(response, resp_size, &used, " %03d %s %d %s %s",
                      eid, event_name, state, event_date, event_time)) {
            closedir(dir);
            return -1;
        }
    }
    closedir(dir);

    if (!appendf(response, resp_size, &used, "\n")) {
        return -1;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
