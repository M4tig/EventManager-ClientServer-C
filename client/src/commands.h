#ifndef COMMANDS_H
#define COMMANDS_H

/**
 * handle_login_command - Process login command
 * @command_line: User input command string
 *
 * Parses UID and password, sends UDP login request to server
 */
void handle_login_command(const char *command_line);

/**
 * handle_change_pass - Process password change command
 * @command_line: User input command string
 *
 * Requires TCP connection, verifies old password and updates to new one
 */
void handle_change_pass(const char *command_line);

/**
 * handle_unregister_command - Process user unregister command
 *
 * Sends UDP request to remove user account from server
 */
void handle_unregister_command(void);

/**
 * handle_logout_command - Process logout command
 *
 * Sends UDP request to log out current user
 */
void handle_logout_command(void);

/**
 * handle_exit_command - Process exit command
 *
 * Checks if user is logged in and exits application
 * Returns: RC_OK on success
 */
int handle_exit_command(void);

/**
 * handle_create_command - Process event creation command
 * @command_line: User input command string
 *
 * Requires TCP connection, sends event details and file to server
 */
void handle_create_command(const char *command_line);

/**
 * handle_close_command - Process event close command
 * @command_line: User input command string
 *
 * Requires TCP connection, closes an event opened by current user
 */
void handle_close_command(const char *command_line);

/**
 * handle_show_command - Process show event command
 * @command_line: User input command string
 *
 * Requires TCP connection, retrieves event details and associated file
 */
void handle_show_command(const char *command_line);

/**
 * handle_myevents_command - Process list user's events command
 *
 * Sends UDP request to list all events created by current user
 */
void handle_myevents_command(void);

/**
 * handle_myreservations_command - Process list user's reservations command
 *
 * Sends UDP request to list all reservations made by current user (max 50)
 */
void handle_myreservations_command(void);

/**
 * handle_list_command - Process list all events command
 *
 * Requires TCP connection, retrieves list of all available events
 */
void handle_list_command(void);

/**
 * handle_reserve_command - Process event reservation command
 * @command_line: User input command string
 *
 * Requires TCP connection, reserves seats for an event
 */
void handle_reserve_command(const char *command_line);

/* Global variables for login state */
extern int logged_in;
extern char current_uid[7];
extern char current_pass[9];

#endif /* COMMANDS_H */
