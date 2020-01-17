/* GStreamer
 * Copyright (C) 2004 Benjamin Otte <otte@gnome.org>
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
#  include "config.h"
#endif

#include <gst/gst.h>

GType gst_break_my_data_get_type (void);
//GType gst_caps_debug_get_type (void);
GType gst_caps_setter_get_type (void);
GType gst_rnd_buffer_size_get_type (void);
GType gst_navseek_get_type (void);
GType gst_progress_report_get_type (void);
GType gst_tag_inject_get_type (void);
GType gst_test_get_type (void);
GType gst_push_file_src_get_type (void);
/*
GType gst_gst_negotiation_get_type (void);
*/
GType gst_cpu_report_get_type (void);

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "breakmydata", GST_RANK_NONE,
          gst_break_my_data_get_type ())
      || !gst_element_register (plugin, "capssetter", GST_RANK_NONE,
          gst_caps_setter_get_type ())
      || !gst_element_register (plugin, "rndbuffersize", GST_RANK_NONE,
          gst_rnd_buffer_size_get_type ())
      || !gst_element_register (plugin, "navseek", GST_RANK_NONE,
          gst_navseek_get_type ())
      || !gst_element_register (plugin, "pushfilesrc", GST_RANK_NONE,
          gst_push_file_src_get_type ()) ||
/*    !gst_element_register (plugin, "negotiation", GST_RANK_NONE, gst_gst_negotiation_get_type ()) || */
      !gst_element_register (plugin, "progressreport", GST_RANK_NONE,
          gst_progress_report_get_type ())
      || !gst_element_register (plugin, "taginject", GST_RANK_NONE,
          gst_tag_inject_get_type ())
      || !gst_element_register (plugin, "testsink", GST_RANK_NONE,
          gst_test_get_type ())
#if 0
      || !gst_element_register (plugin, "capsdebug", GST_RANK_NONE,
          gst_caps_debug_get_type ())
#endif
      || !gst_element_register (plugin, "cpureport", GST_RANK_NONE,
          gst_cpu_report_get_type ()))

    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    debug,
    "elements for testing and debugging",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
