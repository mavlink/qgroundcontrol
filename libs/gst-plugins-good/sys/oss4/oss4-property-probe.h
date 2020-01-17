/* GStreamer OSS4 audio property probe interface implementation
 * Copyright (C) 2007-2008 Tim-Philipp Müller <tim centricular net>
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

#ifndef GST_OSS4_PROPERTY_PROBE_H
#define GST_OSS4_PROPERTY_PROBE_H

#if 0

#include <gst/interfaces/propertyprobe.h>

void      gst_oss4_add_property_probe_interface (GType type);

#endif

gboolean  gst_oss4_property_probe_find_device_name (GstObject   * obj,
                                                    int           fd,
                                                    const gchar * device_handle,
                                                    gchar      ** device_name);

gboolean  gst_oss4_property_probe_find_device_name_nofd (GstObject   * obj,
                                                         const gchar * device_handle,
                                                         gchar      ** device_name);

GValueArray *gst_oss4_property_probe_get_values (GstObject * obj, const gchar * pname);


#endif /* GST_OSS4_PROPERTY_PROBE_H */


