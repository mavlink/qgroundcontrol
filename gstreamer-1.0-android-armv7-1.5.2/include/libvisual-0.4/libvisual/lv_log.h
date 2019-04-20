/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_log.h,v 1.19.2.1 2006/03/04 12:32:47 descender Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_LOG_H
#define _LV_LOG_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <libvisual/lv_defines.h>
#include <libvisual/lvconfig.h>

VISUAL_BEGIN_DECLS

/* This is read-only */
extern char *__lv_progname;

/**
 * Used to determine the severity of the log message using the visual_log
 * define.
 *
 * @see visual_log
 */
typedef enum {
	VISUAL_LOG_DEBUG,	/**< Debug message, to use for debug messages. */
	VISUAL_LOG_INFO,	/**< Informative message, can be used for general info. */
	VISUAL_LOG_WARNING,	/**< Warning message, use to warn the user. */
	VISUAL_LOG_CRITICAL,	/**< Critical message, when a critical situation happens.
				 * Like a NULL pointer passed to a method. */
	VISUAL_LOG_ERROR	/**< Error message, use to notify the user of fatals. 
				 * After the message has been showed, the program is aborted. */
} VisLogSeverity;

/**
 * Used to determine how much verbose the log system should be.
 *
 * @see visual_log_set_verboseness
 */
typedef enum {
	VISUAL_LOG_VERBOSENESS_NONE,	/**< Don't show any message at all. */
	VISUAL_LOG_VERBOSENESS_LOW,	/**< Show only VISUAL_LOG_ERROR and
					  VISUAL_LOG_CRITICAL messages. */
	VISUAL_LOG_VERBOSENESS_MEDIUM,	/**< Show all log messages except VISUAL_LOG_DEBUG ones. */
	VISUAL_LOG_VERBOSENESS_HIGH	/**< Show all log messages. */
} VisLogVerboseness;

/**
 * Functions that want to handle messages must match this signature.
 *
 * @arg message The message that will be shown, exactly the same as that was passed
 * to visual_log(), but after formatting.
 *
 * @arg funcname The name of the function that invokes visual_log(). On non-GNU systems
 * this will probably be NULL.
 *
 * @arg priv Private field to be used by the client. The library will never touch this.
 */
typedef void (*VisLogMessageHandlerFunc) (const char *message,
							const char *funcname, void *priv);

void visual_log_set_verboseness (VisLogVerboseness verboseness);
VisLogVerboseness visual_log_get_verboseness (void);

void visual_log_set_info_handler (VisLogMessageHandlerFunc handler, void *priv);
void visual_log_set_warning_handler (VisLogMessageHandlerFunc handler, void *priv);
void visual_log_set_critical_handler (VisLogMessageHandlerFunc handler, void *priv);
void visual_log_set_error_handler (VisLogMessageHandlerFunc handler, void *priv);

void visual_log_set_all_messages_handler (VisLogMessageHandlerFunc handler, void *priv);

/**
 * Used for log messages, this is brought under a define so
 * that the __FILE__ and __LINE__ macros (and probably __PRETTY_FUNC__) work,
 * and thus provide better information.
 *
 * @see VisLogSeverity
 *
 * @param severity Determines the severity of the message using VisLogSeverity.
 * @param format The format string of the log message.
 */
#ifdef __GNUC__

#ifdef LV_HAVE_ISO_VARARGS
#define visual_log(severity,...)		\
		_lv_log (severity,		\
			__FILE__,		\
			__LINE__,		\
			__PRETTY_FUNCTION__,	\
			__VA_ARGS__)
#elif defined(LV_HAVE_GNUC_VARARGS)
#define visual_log(severity,format...)		\
		_lv_log (severity,		\
			__FILE__,		\
			__LINE__,		\
			__PRETTY_FUNCTION__,	\
			format)
#else

#include <signal.h>

static void visual_log (VisLogSeverity severity, const char *fmt, ...)
{
	char str[1024];
	va_list va;
	char sever_msg[10];
	VisLogVerboseness v;

	assert (fmt != NULL);

	va_start (va, fmt);
	vsnprintf (str, 1023, fmt, va);
	va_end (va);

	switch (severity) {
		case VISUAL_LOG_DEBUG:
			strncpy (sever_msg, "DEBUG", 9);
			break;
		case VISUAL_LOG_INFO:
			strncpy (sever_msg, "INFO", 9);
			break;
		case VISUAL_LOG_WARNING:
			strncpy (sever_msg, "WARNING", 9);
			break;
		case VISUAL_LOG_CRITICAL:
			strncpy (sever_msg, "CRITICAL", 9);
			break;
		case VISUAL_LOG_ERROR:
			strncpy (sever_msg, "ERROR", 9);
			break;
		default:
			assert (0);
	}
	/*
	 * Sorry, we doesn't have (file,line) information
	 */
	v = visual_log_get_verboseness ();
	switch (severity) {
		case VISUAL_LOG_DEBUG:
			if (v == VISUAL_LOG_VERBOSENESS_HIGH)
				fprintf (stderr, "libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			break;
		case VISUAL_LOG_INFO:
			if (v >= VISUAL_LOG_VERBOSENESS_MEDIUM)
				printf ("libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			break;
		case VISUAL_LOG_WARNING:
			if (v >= VISUAL_LOG_VERBOSENESS_MEDIUM)
				fprintf (stderr, "libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			break;
		case VISUAL_LOG_CRITICAL:
			if (v >= VISUAL_LOG_VERBOSENESS_LOW)
				fprintf (stderr, "libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			break;
		case VISUAL_LOG_ERROR:
			if (v >= VISUAL_LOG_VERBOSENESS_LOW)
				printf ("libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			visual_error_raise ();
			break;
	}
}
#endif /* !(ISO_VARARGS || GNUC_VARARGS) */

#endif /* __GNUC__ */


#ifndef __GNUC__

#ifdef LV_HAVE_ISO_VARARGS
#define visual_log(severity,...)		\
		_lv_log (severity,		\
			__FILE__,		\
			__LINE__,		\
			(NULL),			\
			__VA_ARGS__)
#else

#include <signal.h>

static void visual_log (VisLogSeverity severity, const char *fmt, ...)
{
	char str[1024];
	va_list va;
	char sever_msg[10];
	VisLogVerboseness v;

	assert (fmt != NULL);

	va_start (va, fmt);
	vsnprintf (str, 1023, fmt, va);
	va_end (va);

	switch (severity) {
		case VISUAL_LOG_DEBUG:
			strncpy (sever_msg, "DEBUG", 9);
			break;
		case VISUAL_LOG_INFO:
			strncpy (sever_msg, "INFO", 9);
			break;
		case VISUAL_LOG_WARNING:
			strncpy (sever_msg, "WARNING", 9);
			break;
		case VISUAL_LOG_CRITICAL:
			strncpy (sever_msg, "CRITICAL", 9);
			break;
		case VISUAL_LOG_ERROR:
			strncpy (sever_msg, "ERROR", 9);
			break;
		default:
			assert (0);
	}
	/*
	 * Sorry, we don't have (file,line) information
	 */
	v = visual_log_get_verboseness ();
	switch (severity) {
		case VISUAL_LOG_DEBUG:
			if (v == VISUAL_LOG_VERBOSENESS_HIGH)
				fprintf (stderr, "libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			break;
		case VISUAL_LOG_INFO:
			if (v >= VISUAL_LOG_VERBOSENESS_MEDIUM)
				printf ("libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			break;
		case VISUAL_LOG_WARNING:
			if (v >= VISUAL_LOG_VERBOSENESS_MEDIUM)
				fprintf (stderr, "libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			break;
		case VISUAL_LOG_CRITICAL:
			if (v >= VISUAL_LOG_VERBOSENESS_LOW)
				fprintf (stderr, "libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			break;
		case VISUAL_LOG_ERROR:
			if (v >= VISUAL_LOG_VERBOSENESS_LOW)
				printf ("libvisual %s: %s: %s\n",
					sever_msg, __lv_progname, str);
			visual_error_raise ();
			break;
	}
}
#endif /* ISO_VARARGS */

#endif /* !__GNUC__ */

/**
 * Return if @a expr is FALSE, showing a critical message with
 * useful information.
 */
#define visual_log_return_if_fail(expr)				\
	if (expr) { } else					\
	{							\
	visual_log (VISUAL_LOG_CRITICAL,			\
		 "assertion `%s' failed",			\
		#expr);						\
	return;							\
	}

/**
 * Return if @a val if @a expr is FALSE, showing a critical message
 * with useful information.
 */
#define visual_log_return_val_if_fail(expr, val)		\
	if (expr) { } else					\
	{							\
	visual_log (VISUAL_LOG_CRITICAL,			\
		 "assertion `%s' failed",			\
		#expr);						\
	return (val);						\
	}

#if defined __GNUC__
void _lv_log (VisLogSeverity severity, const char *file,
		int line, const char *funcname, const char *fmt, ...)
			__attribute__ ((__format__ (__printf__, 5, 6)));
#else
void _lv_log (VisLogSeverity severity, const char *file,
		int line, const char *funcname, const char *fmt, ...);
#endif

VISUAL_END_DECLS

#endif /* _LV_LOG_H */
