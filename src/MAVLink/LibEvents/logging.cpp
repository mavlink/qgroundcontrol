#include <QtCore/QLoggingCategory>

#include "libevents_includes.h"

Q_STATIC_LOGGING_CATEGORY(EventsLog, "API.Events")

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
