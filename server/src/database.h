#ifndef DATABASE_H
#define DATABASE_H

/*==============================================================================
 *  FILESYSTEM HELPERS
 *==============================================================================*/

/**
 * dir_exists
 * Check if a given path exists and is a directory.
 *
 * Returns 1 if directory exists, 0 otherwise.
 */
int dir_exists(const char *path);

/**
 * dir_is_empty
 * Check if a directory is empty (ignoring "." and "..").
 *
 * Returns 1 if empty or not accessible/not a dir, 0 if it contains entries.
 */
int dir_is_empty(const char *path);

/**
 * get_file_size_bytes
 * Get size of a regular file.
 *
 * Returns 1 on success and writes size to out_size, 0 otherwise.
 */
int get_file_size_bytes(const char *path, size_t *out_size);


/*==============================================================================
 *  VALIDATION
 *==============================================================================*/

/**
 * valid_uid
 * Validate UID format (exactly UID_LEN digits).
 *
 * Returns 1 if valid, 0 otherwise.
 */
int valid_uid(const char *uid);

/**
 * valid_password
 * Validate password format (exactly PASS_LEN alphanumeric chars).
 *
 * Returns 1 if valid, 0 otherwise.
 */
int valid_password(const char *pw);

/**
 * valid_event_name
 * Validate event name (1..NAME_MAX_LEN, alphanumeric only).
 *
 * Returns 1 if valid, 0 otherwise.
 */
int valid_event_name(const char *name);

/**
 * valid_event_date
 * Validate date string in dd-mm-yyyy format, including month/day/leap-year checks.
 *
 * Returns 1 if valid, 0 otherwise.
 */
int valid_event_date(const char *date);

/**
 * valid_event_time
 * Validate time string in hh:mm format.
 *
 * Returns 1 if valid, 0 otherwise.
 */
int valid_event_time(const char *time);

/**
 * valid_attendance
 * Validate attendance value within configured min/max.
 *
 * Returns 1 if valid, 0 otherwise.
 */
int valid_attendance(int attendance);

/**
 * valid_filename
 * Validate filenames used for uploaded description files:
 *  - name part: alnum + '-' + '_'
 *  - extension: dot + 3 letters
 *
 * Returns 1 if valid, 0 otherwise.
 */
int valid_filename(const char *fname);


/*==============================================================================
 *  TIME UTILITIES
 *==============================================================================*/

/**
 * now_time
 * Get current time as time_t (seconds since Unix epoch).
 */
time_t now_time(void);

/**
 * compare_time
 * Compare two timestamps.
 *
 * Returns:
 *  -1 if a < b
 *   0 if a == b
 *  +1 if a > b
 */
int compare_time(time_t a, time_t b);

/**
 * parse_date_time_to_time
 * Convert date + time string to time_t using local time rules.
 *
 * @date: "dd-mm-yyyy"
 * @timestr: "hh:mm" or "hh:mm:ss"
 * @out: output time_t
 * @has_seconds: 0 => parse hh:mm, 1 => parse hh:mm:ss
 *
 * Returns 1 on success, 0 on parse/convert error.
 */
int parse_date_time_to_time(const char *date,
                            const char *timestr,
                            time_t *out,
                            int has_seconds);


/*==============================================================================
 *  USER FILES / ACCOUNT STATE
 *==============================================================================*/

/**
 * user_exists
 * Check if USERS/<uid> directory exists.
 *
 * Returns 1 if exists, 0 otherwise.
 */
int user_exists(const char *uid);

/**
 * user_registered
 * Check if user has a password file USERS/<uid>/<uid>_pass.txt.
 *
 * Returns 1 if registered, 0 otherwise.
 */
int user_registered(const char *uid);

/**
 * user_is_logged_in
 * Check if user login file USERS/<uid>/<uid>_login.txt exists.
 *
 * Returns 1 if logged in, 0 otherwise.
 */
int user_is_logged_in(const char *uid);

/**
 * read_user_password
 * Read stored password into buffer (null-terminated).
 *
 * Returns 1 on success, 0 otherwise.
 */
int read_user_password(const char *uid, char *password, size_t maxlen);

/**
 * create_user_dir
 * Create base user directories:
 *  USERS/<uid>/
 *  USERS/<uid>/CREATED/
 *  USERS/<uid>/RESERVED/
 *
 * Returns 1 on success, 0 otherwise.
 */
int create_user_dir(const char *uid);

/**
 * create_login_file
 * Create USERS/<uid>/<uid>_login.txt indicating logged-in state.
 *
 * Returns 1 on success, 0 otherwise.
 */
int create_login_file(const char *uid);

/**
 * remove_login_file
 * Remove USERS/<uid>/<uid>_login.txt.
 *
 * Returns 1 on success, 0 otherwise.
 */
int remove_login_file(const char *uid);

/**
 * create_password_file
 * Create USERS/<uid>/<uid>_pass.txt storing password.
 *
 * Returns 1 on success, 0 otherwise.
 */
int create_password_file(const char *uid, const char *password);

/**
 * remove_password_file
 * Remove USERS/<uid>/<uid>_pass.txt.
 *
 * Returns 1 on success, 0 otherwise.
 */
int remove_password_file(const char *uid);

/**
 * user_created_events
 * Check if USERS/<uid>/CREATED has at least one event marker.
 *
 * Returns 1 if there are created events, 0 otherwise.
 */
int user_created_events(const char *uid);

/**
 * user_reserved_events
 * Check if USERS/<uid>/RESERVED has at least one reservation file.
 *
 * Returns 1 if there are reservations, 0 otherwise.
 */
int user_reserved_events(const char *uid);


/*==============================================================================
 *  EVENT DIRECTORY / CREATION HELPERS
 *==============================================================================*/

/**
 * server_has_events
 * Check if EVENTS directory contains at least one event.
 *
 * Returns 1 if yes, 0 otherwise.
 */
int server_has_events(void);

/**
 * allocate_eid
 * Find the first free EID in [1..999] (EVENTS/<eid> does not exist).
 *
 * Returns the EID, or 0 if none available.
 */
int allocate_eid(void);

/**
 * create_event_dir
 * Create EVENTS/<eid>/ plus subdirectories:
 *  - DESCRIPTION/
 *  - RESERVATIONS/
 *
 * Returns 1 on success, 0 otherwise.
 */
int create_event_dir(int eid);

/**
 * create_event_start_file
 * Create EVENTS/<eid>/START_<eid>.txt with:
 *  UID name fname date time attendance
 *
 * Returns 1 on success, 0 otherwise.
 */
int create_event_start_file(int eid,
                            const char *uid,
                            const char *name,
                            const char *fname,
                            const char *date,
                            const char *time,
                            int attendance);

/**
 * create_event_res_file
 * Create EVENTS/<eid>/RES_<eid>.txt initialized to 0.
 *
 * Returns 1 on success, 0 otherwise.
 */
int create_event_res_file(int eid);

/**
 * mark_user_created_event
 * Create USERS/<uid>/CREATED/<eid>.txt marker.
 *
 * Returns 1 on success, 0 otherwise.
 */
int mark_user_created_event(const char *uid, int eid);


/*==============================================================================
 *  EVENT GETTERS (read info from event files)
 *==============================================================================*/

/**
 * get_event_name
 * Read event name from START_<eid>.txt.
 *
 * Returns 1 on success, 0 otherwise.
 */
int get_event_name(int eid, char *buffer);

/**
 * get_event_uid
 * Read event creator UID from START_<eid>.txt.
 *
 * Returns 1 on success, 0 otherwise.
 */
int get_event_uid(int eid, char *buffer);

/**
 * get_event_date_time
 * Read event date and time (dd-mm-yyyy / hh:mm) from START_<eid>.txt.
 *
 * Returns 1 on success, 0 otherwise.
 */
int get_event_date_time(int eid, char *date_buffer, char *time_buffer);

/**
 * get_event_desc_fname
 * Read description filename (fname field) from START_<eid>.txt.
 *
 * Returns 1 on success, 0 otherwise.
 */
int get_event_desc_fname(int eid, char *buffer);

/**
 * get_event_max_attendance
 * Read max attendance (capacity) from START_<eid>.txt.
 *
 * Returns capacity on success, -1 on error.
 */
int get_event_max_attendance(int eid);

/**
 * get_event_reservations
 * Read current reserved places from RES_<eid>.txt.
 *
 * Returns current count on success, -1 on error.
 */
int get_event_reservations(int eid);


/*==============================================================================
 *  EVENT STATE / RULES
 *==============================================================================*/

/**
 * event_exists
 * Check if EVENTS/<eid> exists.
 *
 * Returns 1 if exists, 0 otherwise.
 */
int event_exists(int eid);

/**
 * event_is_created_by
 * Check if START_<eid>.txt first field matches uid.
 *
 * Returns 1 if yes, 0 otherwise.
 */
int event_is_created_by(int eid, const char *uid);

/**
 * event_is_closed
 * Check if EVENTS/<eid>/END.txt exists.
 *
 * Returns 1 if closed, 0 otherwise.
 */
int event_is_closed(int eid);

/**
 * event_is_sold_out
 * Compare RES_<eid>.txt against attendance in START_<eid>.txt.
 *
 * Returns 1 if sold out, 0 otherwise.
 */
int event_is_sold_out(int eid);

/**
 * event_date_passed
 * Compare event datetime (from START_<eid>.txt) against now_time().
 *
 * Returns 1 if event time <= now, 0 otherwise.
 */
int event_date_passed(int eid);

/**
 * event_state
 * Compute state code used by your protocol:
 *  1 = future and not sold out
 *  0 = past
 *  2 = future but sold out
 *  3 = closed
 *
 * Returns state [0..3] on success, -1 on invalid eid/unknown.
 */
int event_state(int eid);


/*==============================================================================
 *  EVENT MUTATIONS
 *==============================================================================*/

/**
 * add_reservation
 * Add a reservation to an event:
 *  - updates RES_<eid>.txt
 *  - creates reservation file in EVENTS/<eid>/RESERVATIONS/
 *  - creates reservation file in USERS/<uid>/RESERVED/ (including EID)
 *
 * Returns 1 on success, 0 otherwise.
 */
int add_reservation(int eid, const char *uid, int people);

/**
 * close_event_now
 * Close an event immediately (writes END.txt using current timestamp).
 *
 * Returns 1 on success, 0 otherwise.
 */
int close_event_now(int eid);

/**
 * close_event_with_event_date
 * Close an event using its own scheduled date/time from START_<eid>.txt
 * (writes END.txt with seconds set to ":00").
 *
 * Returns 1 on success, 0 otherwise.
 */
int close_event_with_event_date(int eid);


/*==============================================================================
 *  PROTOCOL COMMAND HELPERS (build response strings)
 *==============================================================================*/

/**
 * list_user_events  (RME)
 * Build "RME ..." response listing events created by uid, with their state.
 *
 * Returns 0 on success, -1 on internal error.
 */
int list_user_events(const char *uid, char *response, size_t resp_size);

/**
 * list_user_reservations  (RMR)
 * Build "RMR ..." response listing up to 50 most recent reservations made by uid,
 * then output-sorted by EID and datetime (oldest first within same EID).
 *
 * Returns 0 on success, -1 on internal error.
 */
int list_user_reservations(const char *uid, char *response, size_t resp_size);

/**
 * list_all_events  (RLS)
 * Build "RLS ..." response listing all events (EID, name, state, date, time).
 *
 * Returns 0 on success, -1 on internal error.
 */
int list_all_events(char *response, size_t resp_size);

#endif /* DATABASE_H */
