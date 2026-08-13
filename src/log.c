/*
 * log.c
 *
 * Aufgabe:
 *   Zentrale Protokollierung der Programmmeldungen.
 */

/* ========================================================================== */
/* Includes                                                                   */
/* ========================================================================== */

#include <stdarg.h>
#include <syslog.h>

#include "log.h"

/* ========================================================================== */
/* Konstanten                                                                 */
/* ========================================================================== */

#define DEFAULT_LOG_LEVEL    LOG_INFO

/* ========================================================================== */
/* Lokale Variablen                                                           */
/* ========================================================================== */

static int current_log_level = DEFAULT_LOG_LEVEL;

/* ========================================================================== */
/* Lokale Funktionen                                                          */
/* ========================================================================== */

static void log_message(int priority,
                        const char *format,
                        va_list arguments)
{
    if (priority > current_log_level)
        return;

    vsyslog(priority, format, arguments);
}

/* ========================================================================== */
/* Öffentliche Funktionen                                                     */
/* ========================================================================== */

void log_init(const char *program_name)
{
    current_log_level = DEFAULT_LOG_LEVEL;

    openlog(program_name, LOG_PID | LOG_NDELAY, LOG_DAEMON);
}

void log_cleanup(void)
{
    closelog();
}

void log_set_level(int level)
{
    switch (level) {

    case LOG_ERR:
    case LOG_WARNING:
    case LOG_NOTICE:
    case LOG_INFO:
    case LOG_DEBUG:
        current_log_level = level;
        break;

    default:
        current_log_level = DEFAULT_LOG_LEVEL;
        break;
    }
}

int log_get_level(void)
{
    return current_log_level;
}

void log_error(const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    log_message(LOG_ERR, format, arguments);
    va_end(arguments);
}

void log_warning(const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    log_message(LOG_WARNING, format, arguments);
    va_end(arguments);
}

void log_notice(const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    log_message(LOG_NOTICE, format, arguments);
    va_end(arguments);
}

void log_info(const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    log_message(LOG_INFO, format, arguments);
    va_end(arguments);
}

void log_debug(const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    log_message(LOG_DEBUG, format, arguments);
    va_end(arguments);
}
