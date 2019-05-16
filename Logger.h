#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declare maximum logging level/verbosity. Lower number indicates higher
 * importance, with the highest importance has level zero. The least
 * important level is five in this implementation, but this can be extended
 * by supplying the appropriate implementation.
 *
 * The level conventions:
 *  - 0: fatal error
 *  - 1: error
 *  - 2: warning
 *  - 3: info
 *  - 4: debug
 *  - 5: verbosity
 *
 * Default: 4
 */
#ifndef LOGGER_MAX_LEVEL
#define LOGGER_MAX_LEVEL   5
#endif

/**
 * Log decoration flag, to be specified with #tk_log_set_decor().
 */
enum log_decoration
{
    LOG_HAS_YEAR       =    1, /**< Include year digit [no]                */
    LOG_HAS_DAY        =    2, /**< Include day of month [no]              */
    LOG_HAS_TIME       =    4, /**< Include time [yes]                     */
    LOG_HAS_SENDER     =    8, /**< Include sender in the log [yes]        */
    LOG_HAS_COLOR      =   16, /**< Colorize logs [yes on win32]           */
    LOG_HAS_LEVEL_TEXT =   32, /**< Include level text string [no]         */
    LOG_HAS_THREAD_ID  =   64, /**< Include thread identification [no]     */
    LOG_HAS_THREAD_SWC =  128, /**< Add mark when thread has switched [yes]*/
    LOG_HAS_CR         =  256, /**< Include carriage return [no]           */
    LOG_HAS_NEWLINE    =  512 /**< Terminate each call with newline [yes] */
};

/**
 * Signature for function to be registered to the logging subsystem to
 * write the actual log message to some output device.
 *
 * @param level     Log level.
 * @param data      Log message, which will be NULL terminated.
 * @param len       Message length.
 */
typedef void logger_func(int level, const char *data, int len);

void logger_set_decor(unsigned decor);
/**
 * Default logging writer function used by front end logger function.
 * This function will print the log message to stdout only.
 * Application normally should NOT need to call this function, but
 * rather use the PJ_LOG macro.
 *
 * @param level     Log level.
 * @param buffer    Log message.
 * @param len       Message length.
 */
void logger_write(int level, const char *buffer, int len);

/**
 * Write to log.
 *
 * @param sender    Source of the message.
 * @param level     Verbosity level.
 * @param format    Format.
 * @param marker    Marker.
 */
void logger_log(const char *sender, int level, const char *format, va_list marker);


/**
 * Change log output function. The front-end logging functions will call
 * this function to write the actual message to the desired device. 
 * By default, the front-end functions use tk_log_write() to write
 * the messages, unless it's changed by calling this function.
 *
 * @param func      The function that will be called to write the log
 *                  messages to the desired device.
 */
void logger_set_func(logger_func *func );

/**
 * Get the current log output function that is used to write log messages.
 *
 * @return          Current log output function.
 */
logger_func* logger_get_func(void);

/**
 * Set maximum log level. Application can call this function to set 
 * the desired level of verbosity of the logging messages. The bigger the
 * value, the more verbose the logging messages will be printed. However,
 * the maximum level of verbosity can not exceed compile time value of
 * PJ_LOG_MAX_LEVEL.
 *
 * @param level     The maximum level of verbosity of the logging
 *                  messages (6=very detailed..1=error only, 0=disabled)
 */
void logger_set_level(int level);

/**
 * Get current maximum log verbositylevel.
 *
 * @return          Current log maximum level.
 */
int logger_get_level(void);

/**
 * format log.
 *
 * @param tag       tag of the message.
 * @param level     Verbosity level.
 * @param format    Format.
 */
void logger_print(const char *tag, int level, const char *format, ...);

/* **************************************************************************/
/*
 * Log functions implementation prototypes.
 * These functions are called by LOG## macros according to verbosity
 * level specified when calling the macro. Applications should not normally
 * need to call these functions directly.
 */
/** fatal error log function. */
#define LOGF(TAG,...) logger_print(TAG, 0, __VA_ARGS__)


/** error log function. */
#define LOGE(TAG,...) logger_print(TAG, 1, __VA_ARGS__)

/** warn log function. */
#define LOGW(TAG,...) logger_print(TAG, 2, __VA_ARGS__)

/** info log function. */
#define LOGI(TAG,...) logger_print(TAG, 3, __VA_ARGS__)

/** debug log function. */
#define LOGD(TAG,...) logger_print(TAG, 4, __VA_ARGS__)

/** verbosity log function. */
#define LOGV(TAG,...) logger_print(TAG, 5, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /*__LOGGER_H__*/