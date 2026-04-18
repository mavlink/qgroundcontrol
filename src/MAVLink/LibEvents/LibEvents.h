#pragma once

#include <cstdlib>
#include <cstdarg>

void qgc_events_debug_printf(const char *fmt, ...);
void qgc_events_parser_debug_printf(const char *fmt, ...);

#define LIBEVENTS_DEBUG_PRINTF qgc_events_debug_printf
#define LIBEVENTS_PARSER_DEBUG_PRINTF qgc_events_parser_debug_printf

#include <MAVLinkLib.h>

#include "protocol/receive.h"
#include "parse/health_and_arming_checks.h"
#include "generated/events_generated.h"
