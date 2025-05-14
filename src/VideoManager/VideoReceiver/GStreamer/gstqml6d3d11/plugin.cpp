/* GStreamer
 * Copyright (C) 2023 Seungha Yang <seungha@centricular.com>
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

#include <gst/gst.h>
#include <gstqml6d3d11sink.h>

GST_DEBUG_CATEGORY (gst_qt6_d3d11_debug);

/**
 * plugin-qt6d3d11:
 *
 * Since: 1.24
 */

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_qt6_d3d11_debug, "qt6d3d11", 0, "qt6d3d11");

  gst_element_register (plugin, "qml6d3d11sink", GST_RANK_NONE,
      GST_TYPE_QML6_D3D11_SINK);

  return TRUE;
}

#ifndef GST_PACKAGE_NAME
#define GST_PACKAGE_NAME   "GStreamer Bad Plug-ins"
#define GST_PACKAGE_ORIGIN "Unknown package origin"
#define GST_LICENSE        "LGPL"
#define PACKAGE            "gst-plugins-bad"
#define PACKAGE_VERSION    "1.24.0"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    qt6d3d11,
    "Qt6 Direct3D11 plugin",
    plugin_init, PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
