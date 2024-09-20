/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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

#include "gstqt6elements.h"
#include "qt6glitem.h"

#include <QtConstructorMacros>

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = FALSE;
// TODO(zdanek) fix after switching to gstreamer 1.20.0+
// original code from 1.20.0
//  ret |= GST_ELEMENT_REGISTER (qml6glsink, plugin);
  ret |= gst_element_register_qml6glsink (plugin);

  return ret;
}

static void registerMetatypes()
{
    qmlRegisterType<Qt6GLVideoItem> ("org.freedesktop.gstreamer.Qt6GLVideoItem", 1, 0, "GstGLQt6VideoItem");
}

Q_CONSTRUCTOR_FUNCTION(registerMetatypes)

#ifndef GST_PACKAGE_NAME
#define GST_PACKAGE_NAME   "GStreamer Bad Plug-ins (qmake)"
#define GST_PACKAGE_ORIGIN "Unknown package origin"
#define GST_LICENSE        "LGPL"
#define PACKAGE            "gst-plugins-bad (qmake)"
#define PACKAGE_VERSION    "1.21.0.1"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    qml6,
    "Qt6 Qml plugin",
    plugin_init, PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN)
