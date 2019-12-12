/* GStreamer
 *
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
 *     @Author: Reynaldo H. Verdejo Pinochet <r.verdejo@sisa.samsung.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more
 */


#include <glib.h>
#include <gst/gst.h>
#include <libsoup/soup.h>
#include "gstsouputils.h"

/*
 * Soup logger funcs
 */

GST_DEBUG_CATEGORY_EXTERN (soup_utils_debug);
#define GST_CAT_DEFAULT soup_utils_debug

static inline gchar
gst_soup_util_log_make_level_tag (SoupLoggerLogLevel level)
{
  gchar c;

  if (G_UNLIKELY ((gint) level > 9))
    return '?';

  switch (level) {
    case SOUP_LOGGER_LOG_MINIMAL:
      c = 'M';
      break;
    case SOUP_LOGGER_LOG_HEADERS:
      c = 'H';
      break;
    case SOUP_LOGGER_LOG_BODY:
      c = 'B';
      break;
    default:
      /* Unknown level. If this is hit libsoup likely added a new
       * log level to SoupLoggerLogLevel and it should be added
       * as a case */
      c = level + '0';
      break;
  }
  return c;
}

static void
gst_soup_util_log_printer_cb (SoupLogger G_GNUC_UNUSED * logger,
    SoupLoggerLogLevel level, char direction, const char *data,
    gpointer user_data)
{
  gchar c;
  c = gst_soup_util_log_make_level_tag (level);
  GST_TRACE_OBJECT (GST_ELEMENT (user_data), "HTTP_SESSION(%c): %c %s", c,
      direction, data);
}

void
gst_soup_util_log_setup (SoupSession * session, SoupLoggerLogLevel level,
    GstElement * element)
{
  SoupLogger *logger;

  if (!level) {
    GST_INFO_OBJECT (element, "Not attaching a logger with level 0");
    return;
  }

  g_assert (session && element);

  if (gst_debug_category_get_threshold (GST_CAT_DEFAULT)
      < GST_LEVEL_TRACE) {
    GST_INFO_OBJECT (element, "Not setting up HTTP session logger. "
        "Need at least GST_LEVEL_TRACE");
    return;
  }

  /* Create a new logger and set body_size_limit to -1 (no limit) */
  logger = soup_logger_new (level, -1);
  soup_logger_set_printer (logger, gst_soup_util_log_printer_cb,
      gst_object_ref (element), (GDestroyNotify) gst_object_unref);

  /* Attach logger to session */
  soup_session_add_feature (session, SOUP_SESSION_FEATURE (logger));
  g_object_unref (logger);
}
