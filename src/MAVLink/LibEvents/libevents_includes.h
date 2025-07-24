/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <cstdlib>
#include <cstdarg>

#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(EventsLog)

void qgc_events_parser_debug_printf(const char *fmt, ...);

//#define LIBEVENTS_PARSER_DEBUG_PRINTF qgc_events_parser_debug_printf
#define LIBEVENTS_DEBUG_PRINTF qgc_events_parser_debug_printf

#include <MAVLinkLib.h>

#include "protocol/receive.h"
#include "parse/health_and_arming_checks.h"
#include "parse/parser.h"
#include "generated/events_generated.h"
