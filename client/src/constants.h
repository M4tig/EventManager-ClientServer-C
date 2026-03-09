#ifndef RC_CLIENT_CONSTANTS_H
#define RC_CLIENT_CONSTANTS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GROUP_NUMBER    53

#define CMD_MAX_LEN       256
#define BUF_SIZE          1024
#define UDP_RESP_MAX     65536
#define TIMEOUT_SECONDS   5

#define UID_MAX_LEN       6
#define PASS_MAX_LEN      8

#define NAME_MAX_LEN      128
#define EVENT_FILE_MAX_LEN 256

#define DEFAULT_ES_PORT    58000 + GROUP_NUMBER

typedef enum {
	CMD_UNKNOWN = 0,
	CMD_LOGIN = 1,            
	CMD_CHANGEPASS = 2,
	CMD_UNREGISTER = 3,
	CMD_LOGOUT = 4,
	CMD_EXIT = 5,
	CMD_CREATE = 6,
	CMD_CLOSE = 7,   
	CMD_MYEVENTS = 8,     
	CMD_LIST = 9,
	CMD_SHOW = 10,  
	CMD_RESERVE = 11,        
	CMD_MYRESERVATIONS = 12     
} command_t;

#define RC_OK        0
#define RC_FAIL      1
#define RC_NOT_AUTH  2

#endif
