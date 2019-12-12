/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstqtsink.h"
#include "gstqtsrc.h"
#include <QtQml/QQmlApplicationEngine>

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "qmlglsink",
          GST_RANK_NONE, GST_TYPE_QT_SINK)) {
    return FALSE;
  }
  
  if (!gst_element_register (plugin, "qmlglsrc",
          GST_RANK_NONE, GST_TYPE_QT_SRC)) {
    return FALSE;
  }
  /* this means the plugin must be loaded before the qml engine is loaded */
  qmlRegisterType<QtGLVideoItem> ("org.freedesktop.gstreamer.GLVideoItem", 1, 0, "GstGLVideoItem");

  return TRUE;
}

#ifndef GST_PACKAGE_NAME
#define GST_PACKAGE_NAME   "GStreamer Bad Plug-ins (qmake)"
#define GST_PACKAGE_ORIGIN "Unknown package origin"
#define GST_LICENSE        "LGPL"
#define PACKAGE            "gst-plugins-bad (qmake)"
#define PACKAGE_VERSION    "1.13.0.1"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    qmlgl,
    "Qt gl plugin",
    plugin_init, PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN)
