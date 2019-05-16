#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>  
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "Logger.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_BUFFER_MAX_SIZE
#define LOG_BUFFER_MAX_SIZE 1024
#endif

typedef struct time_str
{
    /** The seconds part of the time. */
    long    sec;

    /** The miliseconds fraction of the time. */
    long    msec;

} time_str;

/**
 * This structure represent the parsed representation of time.
 * It is acquired by calling #pj_time_decode().
 */
typedef struct detail_time_str
{
    /** This represents day of week where value zero means Sunday */
    int wday;

    /* This represents day of the year, 0-365, where zero means
     *  1st of January.
     */
    /*int yday; */

    /** This represents day of month: 1-31 */
    int day;

    /** This represents month, with the value is 0 - 11 (zero is January) */
    int mon;

    /** This represent the actual year (unlike in ANSI libc where
     *  the value must be added by 1900).
     */
    int year;

    /** This represents the second part, with the value is 0-59 */
    int sec;

    /** This represents the minute part, with the value is: 0-59 */
    int min;

    /** This represents the hour part, with the value is 0-23 */
    int hour;

    /** This represents the milisecond part, with the value is 0-999 */
    int msec;

} detail_time_str;

static int log_max_level = LOGGER_MAX_LEVEL;

static logger_func *log_writer_fun = &logger_write;

static unsigned log_decor = LOG_HAS_DAY |
                            LOG_HAS_TIME |
                            LOG_HAS_SENDER |
                            LOG_HAS_LEVEL_TEXT |
                            LOG_HAS_THREAD_ID |
                            LOG_HAS_NEWLINE;

static int inner_gettimeofday(time_str *p_tv)
{
    struct timeval the_time;
    int rc;

    rc = gettimeofday(&the_time, NULL);
    if (rc != 0)
        return rc;

    p_tv->sec = the_time.tv_sec;
    p_tv->msec = the_time.tv_usec / 1000;
    return 0;
}

static int inner_time_decode(const time_str *tv, detail_time_str *pt)
{
    struct tm local_time;

    localtime_r((time_t*)&tv->sec, &local_time);

    pt->year = local_time.tm_year+1900;
    pt->mon = local_time.tm_mon;
    pt->day = local_time.tm_mday;
    pt->hour = local_time.tm_hour;
    pt->min = local_time.tm_min;
    pt->sec = local_time.tm_sec;
    pt->wday = local_time.tm_wday;
    pt->msec = tv->msec;

    return 0;
}

/**
 * Encode parsed time to time value.
 */
static int inner_time_encode(const detail_time_str *pt, time_str *tv)
{
    struct tm local_time;

    local_time.tm_year = pt->year-1900;
    local_time.tm_mon = pt->mon;
    local_time.tm_mday = pt->day;
    local_time.tm_hour = pt->hour;
    local_time.tm_min = pt->min;
    local_time.tm_sec = pt->sec;
    local_time.tm_isdst = 0;
    
    tv->sec = mktime(&local_time);
    tv->msec = pt->msec;

    return 0;
}

static int inner_getpid()
{
    return getpid();
}

static int inner_gettid()
{
    return syscall(SYS_gettid);
}

static int inner_utoa_pad( unsigned long val, char *buf, int min_dig, int pad)
{
    char *p;
    int len;

    p = buf;
    do {
        unsigned long digval = (unsigned long) (val % 10);
        val /= 10;
        *p++ = (char) (digval + '0');
    } while (val > 0);

    len = (int)(p-buf);
    while (len < min_dig) {
        *p++ = (char)pad;
        ++len;
    }
    *p-- = '\0';

    do {
        char temp = *p;
        *p = *buf;
        *buf = temp;
        --p;
        ++buf;
    } while (buf < p);

    return len;
}

static int inner_utoa(unsigned long val, char *buf)
{
    return inner_utoa_pad(val, buf, 0, 0);
}

void logger_set_level(int level)
{
    log_max_level = level;
}

int logger_get_level(void)
{
    return log_max_level;
}

void logger_set_decor(unsigned decor)
{
    log_decor = decor;
}

void logger_set_log_func(logger_func *func )
{
    log_writer_fun = func;
}

logger_func* logger_get_func(void)
{                                  
    return log_writer_fun;
}

void logger_log( const char *sender, int level, const char *format, va_list marker)
{
    time_str now;
    detail_time_str ptime;
    char *pre;
    char log_buffer[LOG_BUFFER_MAX_SIZE];

    int saved_level, len, print_len, indent;

    if (level > log_max_level)
        return;

    /* Get current date/time. */
    inner_gettimeofday(&now);
    inner_time_decode(&now, &ptime);

    pre = log_buffer;
    if (log_decor & LOG_HAS_YEAR) {
        pre += inner_utoa(ptime.year, pre);
    }
    if (log_decor & LOG_HAS_DAY) {
        if (pre!=log_buffer) *pre++ = '-';
        pre += inner_utoa_pad(ptime.mon+1, pre, 2, '0');
        *pre++ = '-';
        pre += inner_utoa_pad(ptime.day, pre, 2, '0');
    }
    if (log_decor & LOG_HAS_TIME) {
        if (pre!=log_buffer) *pre++ = ' ';
        pre += inner_utoa_pad(ptime.hour, pre, 2, '0');
        *pre++ = ':';
        pre += inner_utoa_pad(ptime.min, pre, 2, '0');
        *pre++ = ':';
        pre += inner_utoa_pad(ptime.sec, pre, 2, '0');
        *pre++ = '.';
        pre += inner_utoa_pad(ptime.msec, pre, 3, '0');
    }
    if (log_decor & LOG_HAS_THREAD_ID) {
        char threadid[10];
        char *id = threadid;
        int thread_len;
        enum { THREAD_WIDTH = 6 };
        if (pre!=log_buffer) {
            *pre++ = ' ';
            *pre++ = ' ';
        }
        int pid = inner_getpid();
        thread_len = inner_utoa(pid,threadid);
        if (thread_len <= THREAD_WIDTH) {
            while (thread_len < THREAD_WIDTH)
                *pre++ = ' ', ++thread_len;
            while (*id)
                *pre++ = *id++;
        } else {
            int i;
            for (i=0; i<THREAD_WIDTH; ++i)
                *pre++ = *id++;
        }
        *pre++ = ' ';
        *pre++ = ' ';
        memset(threadid, 0, sizeof(threadid));
        int tid = inner_gettid();
        thread_len = inner_utoa(tid,threadid);
        id = threadid;
        if (thread_len <= THREAD_WIDTH) {
            while (thread_len < THREAD_WIDTH)
                *pre++ = ' ', ++thread_len;
            while (*id)
                *pre++ = *id++;
        } else {
            int i;
            for (i=0; i<THREAD_WIDTH; ++i)
                *pre++ = *id++;
        }
    }
    if (log_decor & LOG_HAS_LEVEL_TEXT) {
        if (pre!=log_buffer) *pre++ = ' ';
        static const char *ltexts[] = { "F", "E", "W", 
                              "I", "D", "V"};
        strcpy(pre, ltexts[level]);
        pre += 1;
    }

    if (log_decor & LOG_HAS_SENDER) {
        enum { SENDER_WIDTH = 25 };
        size_t sender_len = strlen(sender);
        if (pre!=log_buffer) *pre++ = ' ';
        if (sender_len <= SENDER_WIDTH) {
            while (*sender)
                *pre++ = *sender++;
        } else {
            int i;
            for (i=0; i<SENDER_WIDTH; ++i)
                *pre++ = *sender++;
        }
        *pre++ = ':';
    }

    len = (int)(pre - log_buffer);

    /* Print the whole message to the string log_buffer. */
    print_len = vsnprintf(pre, sizeof(log_buffer)-len, format, marker);
    if (print_len < 0) {
        level = 1;
        print_len = snprintf(pre, sizeof(log_buffer)-len, "<logging error: msg too long>");
    }
    if (print_len < 1 || print_len >= (int)(sizeof(log_buffer)-len)) {
        print_len = sizeof(log_buffer) - len - 1;
    }
    len = len + print_len;
    if (len > 0 && len < (int)sizeof(log_buffer)-2) {
        if (log_decor & LOG_HAS_CR) {
            log_buffer[len++] = '\r';
        }
        if (log_decor & LOG_HAS_NEWLINE) {
            log_buffer[len++] = '\n';
        }
        log_buffer[len] = '\0';
    } else {
        len = sizeof(log_buffer)-1;
        if (log_decor & LOG_HAS_CR) {
            log_buffer[sizeof(log_buffer)-3] = '\r';
        }
        if (log_decor & LOG_HAS_NEWLINE) {
            log_buffer[sizeof(log_buffer)-2] = '\n';
        }
        log_buffer[sizeof(log_buffer)-1] = '\0';
    }

    if (log_writer_fun)
        (*log_writer_fun)(level, log_buffer, len);
}

void logger_print(const char *tag, int level, const char *format, ...)
{
    if (logger_get_level() >= level) {
        va_list arg;
        va_start(arg, format);
        logger_log(tag, level, format, arg);
        va_end(arg);
    }
}

#ifdef __cplusplus
}
#endif