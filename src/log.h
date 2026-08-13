/*
 * log.h
 *
 * Aufgabe:
 *   Öffentliche Schnittstelle für die zentrale Protokollierung.
 */

#ifndef LOG_H
#define LOG_H

/* ========================================================================== */
/* Includes                                                                   */
/* ========================================================================== */

#include <syslog.h>

/* ========================================================================== */
/* Öffentliche Funktionen                                                     */
/* ========================================================================== */

void log_init(const char *program_name);
void log_cleanup(void);

void log_set_level(int level);
int  log_get_level(void);

void log_error(const char *format, ...);
void log_warning(const char *format, ...);
void log_notice(const char *format, ...);
void log_info(const char *format, ...);
void log_debug(const char *format, ...);

#endif /* LOG_H */
