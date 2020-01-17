/* GStreamer
 * Copyright (C) 2007 Julien Puydt <jpuydt@free.fr>
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

#include <libavc1394/avc1394.h>
#include <libavc1394/avc1394_vcr.h>
#include <libavc1394/rom1394.h>
#include <libraw1394/raw1394.h>

#include <gst/gst.h>

#include "gst1394probe.h"

#if 0

static GValueArray *
gst_1394_get_guid_array (void)
{
  GValueArray *result = NULL;
  raw1394handle_t handle = NULL;
  int num_ports = 0;
  int port = 0;
  int num_nodes = 0;
  int node = 0;
  rom1394_directory directory;
  GValue value = { 0, };

  handle = raw1394_new_handle ();

  if (handle == NULL)
    return NULL;

  num_ports = raw1394_get_port_info (handle, NULL, 0);
  for (port = 0; port < num_ports; port++) {
    if (raw1394_set_port (handle, port) >= 0) {
      num_nodes = raw1394_get_nodecount (handle);
      for (node = 0; node < num_nodes; node++) {
        rom1394_get_directory (handle, node, &directory);
        if (rom1394_get_node_type (&directory) == ROM1394_NODE_TYPE_AVC &&
            avc1394_check_subunit_type (handle, node,
                AVC1394_SUBUNIT_TYPE_VCR)) {
          if (result == NULL)
            result = g_value_array_new (3);     /* looks like a sensible default */
          g_value_init (&value, G_TYPE_UINT64);
          g_value_set_uint64 (&value, rom1394_get_guid (handle, node));
          g_value_array_append (result, &value);
          g_value_unset (&value);
        }
      }
    }
  }

  return result;
}

static const GList *
gst_1394_property_probe_get_properties (GstPropertyProbe * probe)
{
  static GList *result = NULL;
  GObjectClass *klass = NULL;
  GParamSpec *spec = NULL;

  if (result == NULL) {
    klass = G_OBJECT_GET_CLASS (probe);
    spec = g_object_class_find_property (klass, "guid");
    result = g_list_append (result, spec);
  }

  return result;
}

static void
gst_1394_property_probe_probe_property (GstPropertyProbe * probe, guint prop_id,
    const GParamSpec * pspec)
{
  if (!g_str_equal (pspec->name, "guid"))
    G_OBJECT_WARN_INVALID_PROPERTY_ID (probe, prop_id, pspec);
}

static gboolean
gst_1394_property_probe_needs_probe (GstPropertyProbe * probe, guint prop_id,
    const GParamSpec * pspec)
{
  return TRUE;
}

static GValueArray *
gst_1394_property_probe_get_values (GstPropertyProbe * probe, guint prop_id,
    const GParamSpec * pspec)
{
  GValueArray *result = NULL;

  if (!g_str_equal (pspec->name, "guid")) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (probe, prop_id, pspec);
    return NULL;
  }

  result = gst_1394_get_guid_array ();

  if (result == NULL)
    GST_LOG_OBJECT (probe, "No guid found");

  return result;
}

static void
gst_1394_property_probe_interface_init (GstPropertyProbeInterface * iface)
{
  iface->get_properties = gst_1394_property_probe_get_properties;
  iface->probe_property = gst_1394_property_probe_probe_property;
  iface->needs_probe = gst_1394_property_probe_needs_probe;
  iface->get_values = gst_1394_property_probe_get_values;
}

void
gst_1394_type_add_property_probe_interface (GType type)
{
  static const GInterfaceInfo probe_iface_info = {
    (GInterfaceInitFunc) gst_1394_property_probe_interface_init,
    NULL,
    NULL,
  };

  g_type_add_interface_static (type, GST_TYPE_PROPERTY_PROBE,
      &probe_iface_info);
}

#endif
