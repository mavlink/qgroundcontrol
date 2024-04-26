/****************************************************************************
 *
 * (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <cstdlib>
#include <cstdarg>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(EventsLog)

void qgc_events_parser_debug_printf(const char *fmt, ...);

//#define LIBEVENTS_PARSER_DEBUG_PRINTF qgc_events_parser_debug_printf
#define LIBEVENTS_DEBUG_PRINTF qgc_events_parser_debug_printf

#include <MAVLinkProtocol.h>

#include "libevents/libs/cpp/protocol/receive.h"
#include "libevents/libs/cpp/parse/health_and_arming_checks.h"
#include "libevents/libs/cpp/parse/parser.h"
#include "libevents/libs/cpp/generated/events_generated.h"