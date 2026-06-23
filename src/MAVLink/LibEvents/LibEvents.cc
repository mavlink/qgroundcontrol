#include "LibEvents.h"

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(LibEventsLog, "MAVLink.LibEvents.LibEvents")
QGC_LOGGING_CATEGORY(LibEventsParserLog, "MAVLink.LibEvents.Parser")

void qgc_events_debug_printf(const char *fmt, ...)
{
    char msg[256];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);
    msg[sizeof(msg)-1] = '\0';
    const int len = strlen(msg);
    if (len > 0) {
        msg[len-1] = '\0'; // remove newline
    }
    qCDebug(LibEventsLog) << msg;
}

void qgc_events_parser_debug_printf(const char *fmt, ...)
{
    char msg[256];
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);
    msg[sizeof(msg)-1] = '\0';
    const int len = strlen(msg);
    if (len > 0) {
        msg[len-1] = '\0'; // remove newline
    }
    qCDebug(LibEventsParserLog) << msg;
}
