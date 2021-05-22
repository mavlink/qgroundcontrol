/****************************************************************************
 *
 * (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "libevents_definitions.h"

#include <QGCLoggingCategory.h>

QGC_LOGGING_CATEGORY(EventsLog, "EventsLog");

void qgc_events_parser_debug_printf(const char *fmt, ...) {
    char msg[256];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);
    msg[sizeof(msg)-1] = '\0';
    int len = strlen(msg);
    if (len > 0) msg[len-1] = '\0'; // remove newline
    qCDebug(EventsLog) << msg;
}

