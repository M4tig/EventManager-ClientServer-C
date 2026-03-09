#ifndef RC_SERVER_CONSTANTS_H
#define RC_SERVER_CONSTANTS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GROUP_NUMBER    53

#define CMD_MAX_LEN       256
#define BUF_SIZE          1024
#define MAX_BUF_SIZE      65536

#define UID_LEN           6
#define PASS_LEN          8
#define EID_LEN           3
#define NAME_MAX_LEN      10
#define FNAME_MAX_LEN     256
#define DATE_LEN          10
#define TIME_LEN          5
#define DATETIME_LEN      16

#define MAX_FILE_SIZE     (10L * 1024L * 1024L)  /* 10 MB */
#define LIST_RESPONSE_SIZE (128L * 1024L)  /* 128 KB */
#define MIN_ATTENDANCE    10
#define MAX_ATTENDANCE    999
#define MAX_RESERVATIONS  50

#define DEFAULT_ES_PORT   (58000 + GROUP_NUMBER)

/* Return codes */
#define RC_OK        0
#define RC_FAIL      1
#define RC_ERR       2

#endif
