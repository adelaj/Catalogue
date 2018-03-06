#pragma once

#include "common.h"

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <tchar.h>
#include <windows.h>

struct log {
    FILE *file;
    TCHAR *buff;
    size_t buff_cap, file_sz;
};

enum message_type { 
    MESSAGE_TYPE_DEFAULT = 0,
    MESSAGE_TYPE_ERROR,
    MESSAGE_TYPE_WARNING,
    MESSAGE_TYPE_NOTE,
    MESSAGE_TYPE_INFO,
    MESSAGE_TYPE_RESERVED
};

typedef size_t (*message_callback)(TCHAR *, size_t, void *);
typedef size_t (*message_callback_var)(TCHAR *, size_t, void *, TCHAR *, va_list);

struct message {
    const TCHAR *path;
    const char *func;
    size_t line;
    union {
        message_callback handler;
        message_callback_var handler_var;
    };
    enum message_type type;
};

struct message_info_time_diff {
    struct message base;
    uint64_t start, stop;
    TCHAR *descr;
};

size_t message_info_time_diff(TCHAR *, size_t, struct message_info_time_diff *);

struct message_error_crt {
    struct message base;
    errno_t code;
};

struct message_error_wapi {
    struct message base;
    DWORD code;
};

union message_error {
    struct message base;
    struct message_error_crt error_crt;
    struct message_error_wapi error_wapi;
};

size_t message_error_crt(TCHAR *, size_t, struct message_error_crt *);
size_t message_error_wapi(TCHAR *, size_t, struct message_error_wapi *);
size_t message_var_generic(TCHAR *, size_t, void *, TCHAR *, va_list);

#define DECLARE_PATH \
    static const TCHAR Path[] = _T(__FILE__);    

#define MESSAGE(Handler, Type) \
    (struct message) { \
        .path = Path, \
        .func = __func__, \
        .line = __LINE__, \
        .handler = (message_callback) (Handler), \
        .type = (Type) \
    }

#define MESSAGE_VAR(Handler_var, Type) \
    (struct message) { \
        .path = Path, \
        .func = __func__, \
        .line = __LINE__, \
        .handler_var = (message_callback_var) (Handler_var), \
        .type = (Type) \
    }

#define MESSAGE_INFO_TIME_DIFF(Start, Stop, Descr) \
    ((struct message_info_time_diff) { \
        .base = MESSAGE(message_info_time_diff, MESSAGE_TYPE_INFO), \
        .start = (Start), \
        .stop = (Stop), \
        .descr = (Descr) \
    })

#define MESSAGE_ERROR_CRT(Code) \
    ((struct message_error_crt) { \
        .base = MESSAGE(message_error_crt, MESSAGE_TYPE_ERROR), \
        .code = (Code) \
    })

#define MESSAGE_ERROR_WAPI(Code) \
    ((struct message_error_wapi) { \
        .base = MESSAGE(message_error_wapi, MESSAGE_TYPE_ERROR), \
        .code = (Code) \
    })

#define MESSAGE_VAR_GENERIC(Type) \
    (MESSAGE_VAR(message_var_generic, (Type)))

bool log_init(struct log *, TCHAR *, size_t);
void log_close(struct log *);
bool log_message(struct log *, struct message *);
bool log_message_var(struct log *, struct message *, TCHAR *, ...);
