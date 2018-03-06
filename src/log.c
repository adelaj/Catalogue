#include "log.h"
#include "memory.h"

#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

size_t message_info_time_diff(TCHAR *buff, size_t buff_cap, struct message_info_time_diff *context)
{
    int res = 0;
    if (context->stop >= context->start)
    {
        uint64_t diff = context->stop - context->start;
        uint64_t hdq = diff / 3600000000, hdr = diff % 3600000000, mdq = hdr / 60000000, mdr = hdr % 60000000;
        double sec = (double) mdr / 1.e6;
        res =
            hdq ? _sntprintf(buff, buff_cap, _T("%s took %") _T(PRIu64) _T(" hr %") _T(PRIu64) _T(" min %.4f sec.\n"), context->descr, hdq, mdq, sec) :
            mdq ? _sntprintf(buff, buff_cap, _T("%s took %") _T(PRIu64) _T(" min %.4f sec.\n"), context->descr, mdq, sec) :
            _sntprintf(buff, buff_cap, _T("%s took %.4f sec.\n"), context->descr, sec);
    }
    else
        res = _sntprintf(buff, buff_cap, _T("%s took too much time.\n"), context->descr);
    return (size_t) MAX(res, 0);
}

size_t message_error_crt(TCHAR *buff, size_t buff_cap, struct message_error_crt *context)
{
    if (!_tcserror_s(buff, buff_cap, context->code))
    {
        size_t len = _tcsnlen(buff, buff_cap);
        int tmp = _sntprintf(buff + len, buff_cap - len, _T("!\n"));
        if (tmp > 0 && (size_t) tmp < buff_cap - len)
        {
            len += (size_t) tmp;
            return len;
        }        
    }
    return 0;
}

size_t message_error_wapi(TCHAR *buff, size_t buff_cap, struct message_error_wapi *context)
{
    size_t len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, context->code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buff, (DWORD) buff_cap, NULL);
    if (len &&  len < buff_cap) return len;
    return 0;
}

size_t message_var_generic(TCHAR *buff, size_t buff_cap, void *context, TCHAR *format, va_list arg)
{
    (void) context;
    int tmp = _vsntprintf(buff, buff_cap, format, arg);
    if (tmp > 0 && (size_t) tmp < buff_cap) return (size_t) tmp;
    return 0;
}

bool log_init(struct log *log, TCHAR *path, size_t buff_cap)
{
    if (array_init(&log->buff, &buff_cap, sizeof(*log->buff), 0, 0))
    {
        log->buff_cap = buff_cap;
        if (path)
        {
            log->file = _tfopen(path, _T("w"));
            if (log->file) return 1;
        }
        else
        {
            log->file = stderr;
            return 1;
        }
        free(log->buff);
    }
    return 0;
}

void log_close(struct log *p_log)
{
    if (p_log->file && p_log->file != stderr) fclose(p_log->file);
    free(p_log->buff);
}

static size_t log_prefix(TCHAR *buff, size_t buff_cap, enum message_type type, const char *func, const TCHAR *path, size_t line)
{
    time_t t;
    time(&t);
    struct tm ts;
    localtime_s(&ts, &t);
    TCHAR *title[] = { _T("MESSAGE"), _T("ERROR"), _T("WARNING"), _T("NOTE"), _T("INFO") };
    size_t len = _tcsftime(buff, buff_cap, _T("[%Y-%m-%d %H:%M:%S UTC%z] "), &ts);
    if (len)
    {
        int tmp = _sntprintf(buff + len, buff_cap - len, _T("%s (%hs @ \"%s\":%zu): "), title[type], func, path, line);
        if (tmp > 0 && (size_t) tmp < buff_cap - len) return len + (size_t) tmp;
    }
    return SIZE_MAX;
}

bool log_message(struct log *log, struct message *message)
{
    if (!log) return 1;

    size_t len = log_prefix(log->buff, log->buff_cap, message->type, message->func, message->path, message->line);
    if (len < log->buff_cap)
    {
        size_t tmp = message->handler(log->buff + len, log->buff_cap - len, message);
        if (tmp < log->buff_cap - len)
        {
            len += tmp;
            size_t wr = fwrite(log->buff, sizeof(*log->buff), len, log->file);
            log->file_sz += wr;                    
            if (wr == len) return 1;
        }
    }
    return 0;
}

bool log_message_var(struct log *log, struct message *message, TCHAR *format, ...)
{
    if (!log) return 1;

    size_t len = log_prefix(log->buff, log->buff_cap, message->type, message->func, message->path, message->line);
    if (len < log->buff_cap)
    {
        va_list arg;
        va_start(arg, format);
        size_t tmp = message->handler_var(log->buff + len, log->buff_cap - len, message, format, arg);
        va_end(arg);
        if (tmp < log->buff_cap - len)
        {
            len += tmp;
            size_t wr = fwrite(log->buff, sizeof(*log->buff), len, log->file);
            log->file_sz += wr;
            if (wr == len) return 1;
        }
    }
    return 0;
}
