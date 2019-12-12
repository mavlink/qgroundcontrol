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

/* FIXME 0.11: suppress warnings for deprecated API such as GValueArray
 * with newer GLib versions (>= 2.31.0) */
#define GLIB_DISABLE_DEPRECATION_WARNINGS

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>


#define NO_LEGACY_MIXER
#include "oss4-audio.h"
#include "oss4-sink.h"
#include "oss4-source.h"
#include "oss4-soundcard.h"
#include "oss4-property-probe.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#if 0

GST_DEBUG_CATEGORY_EXTERN (oss4_debug);
#define GST_CAT_DEFAULT oss4_debug

static const GList *
gst_oss4_property_probe_get_properties (GstPropertyProbe * probe)
{
  GObjectClass *klass = G_OBJECT_GET_CLASS (probe);
  GList *list;

  GST_OBJECT_LOCK (GST_OBJECT (probe));

  /* we create a new list and store it in the instance struct, since apparently
   * we forgot to update the API for 0.10 (and why don't we mark probable
   * properties with a flag instead anyway?). A bit hackish, but what can you
   * do (can't really use a static variable since the pspec will be different
   * for src and sink class); this isn't particularly pretty, but the best
   * we can do given that we can't create a common base class (we could do
   * fancy things with the interface, or use g_object_set_data instead, but
   * it's not really going to make it much better) */
  if (GST_IS_AUDIO_SINK_CLASS (klass)) {
    list = GST_OSS4_SINK (probe)->property_probe_list;
  } else if (GST_IS_AUDIO_SRC_CLASS (klass)) {
    list = GST_OSS4_SOURCE (probe)->property_probe_list;
  } else if (GST_IS_OSS4_MIXER_CLASS (klass)) {
    list = GST_OSS4_MIXER (probe)->property_probe_list;
  } else {
    GST_OBJECT_UNLOCK (GST_OBJECT (probe));
    g_return_val_if_reached (NULL);
  }

  if (list == NULL) {
    GParamSpec *pspec;

    pspec = g_object_class_find_property (klass, "device");
    list = g_list_prepend (NULL, pspec);

    if (GST_IS_AUDIO_SINK_CLASS (klass)) {
      GST_OSS4_SINK (probe)->property_probe_list = list;
    } else if (GST_IS_AUDIO_SRC_CLASS (klass)) {
      GST_OSS4_SOURCE (probe)->property_probe_list = list;
    } else if (GST_IS_OSS4_MIXER_CLASS (klass)) {
      GST_OSS4_MIXER (probe)->property_probe_list = list;
    }
  }

  GST_OBJECT_UNLOCK (GST_OBJECT (probe));

  return list;
}

static void
gst_oss4_property_probe_probe_property (GstPropertyProbe * probe,
    guint prop_id, const GParamSpec * pspec)
{
  if (!g_str_equal (pspec->name, "device")) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID (probe, prop_id, pspec);
  }
}

static gboolean
gst_oss4_property_probe_needs_probe (GstPropertyProbe * probe,
    guint prop_id, const GParamSpec * pspec)
{
  /* don't cache probed data */
  return TRUE;
}

#endif

/* caller must ensure LOCK is taken (e.g. if ioctls need to be serialised) */
gboolean
gst_oss4_property_probe_find_device_name (GstObject * obj, int fd,
    const gchar * device_handle, gchar ** device_name)
{
  struct oss_sysinfo si = { {0,}, };
  gchar *name = NULL;

  if (ioctl (fd, SNDCTL_SYSINFO, &si) == 0) {
    int i;

    for (i = 0; i < si.numaudios; ++i) {
      struct oss_audioinfo ai = { 0, };

      ai.dev = i;
      if (ioctl (fd, SNDCTL_AUDIOINFO, &ai) == -1) {
        GST_DEBUG_OBJECT (obj, "AUDIOINFO ioctl for device %d failed", i);
        continue;
      }
      if (strcmp (ai.devnode, device_handle) == 0) {
        name = g_strdup (ai.name);
        break;
      }
    }
  } else {
    GST_WARNING_OBJECT (obj, "SYSINFO ioctl failed: %s", g_strerror (errno));
  }

  /* try ENGINEINFO as fallback (which is better than nothing) */
  if (name == NULL) {
    struct oss_audioinfo ai = { 0, };

    GST_LOG_OBJECT (obj, "device %s not listed in AUDIOINFO", device_handle);
    ai.dev = -1;
    if (ioctl (fd, SNDCTL_ENGINEINFO, &ai) == 0)
      name = g_strdup (ai.name);
  }

  GST_DEBUG_OBJECT (obj, "Device name: %s", GST_STR_NULL (name));

  if (name != NULL)
    *device_name = name;

  return (name != NULL);
}

gboolean
gst_oss4_property_probe_find_device_name_nofd (GstObject * obj,
    const gchar * device_handle, gchar ** device_name)
{
  gboolean res;
  int fd;

  fd = open ("/dev/mixer", O_RDONLY);
  if (fd < 0)
    return FALSE;

  res = gst_oss4_property_probe_find_device_name (obj, fd, device_handle,
      device_name);

  close (fd);
  return res;
}

static GList *
gst_oss4_property_probe_get_audio_devices (GstObject * obj, int fd,
    struct oss_sysinfo *si, int cap_mask)
{
  GList *devices = NULL;
  int i;

  GST_LOG_OBJECT (obj, "%d audio/dsp devices", si->numaudios);

  for (i = 0; i < si->numaudios; ++i) {
    struct oss_audioinfo ai = { 0, };

    ai.dev = i;
    if (ioctl (fd, SNDCTL_AUDIOINFO, &ai) == -1) {
      GST_DEBUG_OBJECT (obj, "AUDIOINFO ioctl for device %d failed", i);
      continue;
    }

    if ((ai.caps & cap_mask) == 0) {
      GST_DEBUG_OBJECT (obj, "audio device %d is not an %s device", i,
          (cap_mask == PCM_CAP_OUTPUT) ? "output" : "input");
      continue;
    }

    if (!ai.enabled) {
      GST_DEBUG_OBJECT (obj, "audio device %d is not usable/enabled", i);
      continue;
    }

    GST_DEBUG_OBJECT (obj, "audio device %d looks ok: %s (\"%s\")", i,
        ai.devnode, ai.name);

    devices = g_list_prepend (devices, g_strdup (ai.devnode));
  }

  return g_list_reverse (devices);
}

GValueArray *
gst_oss4_property_probe_get_values (GstObject * probe, const gchar * pname)
{
  struct oss_sysinfo si = { {0,}, };
  GValueArray *array = NULL;
  GstObject *obj;
  GList *devices, *l;
  int cap_mask, fd = -1;

  if (!g_str_equal (pname, "device")) {
    GST_WARNING_OBJECT (probe, "invalid property");
    return NULL;
  }

  obj = GST_OBJECT (probe);

  GST_OBJECT_LOCK (obj);

  /* figure out whether the element is a source or sink */
  if (GST_IS_OSS4_SINK (probe)) {
    GST_DEBUG_OBJECT (probe, "probing available output devices");
    cap_mask = PCM_CAP_OUTPUT;
    fd = GST_OSS4_SINK (probe)->fd;
  } else if (GST_IS_OSS4_SOURCE (probe)) {
    GST_DEBUG_OBJECT (probe, "probing available input devices");
    cap_mask = PCM_CAP_INPUT;
    fd = GST_OSS4_SOURCE (probe)->fd;
  } else {
    GST_OBJECT_UNLOCK (obj);
    g_assert_not_reached ();
    return NULL;
  }

  /* copy fd if it's open, so we can just unconditionally close() later */
  if (fd != -1)
    fd = dup (fd);

  /* this will also catch the unlikely case where the above dup() failed */
  if (fd == -1) {
    fd = open ("/dev/mixer", O_RDONLY | O_NONBLOCK, 0);
    if (fd < 0)
      goto open_failed;
    else if (!gst_oss4_audio_check_version (GST_OBJECT (probe), fd))
      goto legacy_oss;
  }

  if (ioctl (fd, SNDCTL_SYSINFO, &si) == -1)
    goto no_sysinfo;

  devices = gst_oss4_property_probe_get_audio_devices (obj, fd, &si, cap_mask);

  if (devices == NULL) {
    GST_OBJECT_UNLOCK (obj);
    GST_DEBUG_OBJECT (obj, "No devices found");
    goto done;
  }

  array = g_value_array_new (1);

  for (l = devices; l != NULL; l = l->next) {
    GValue val = { 0, };

    g_value_init (&val, G_TYPE_STRING);
    g_value_take_string (&val, (gchar *) l->data);
    l->data = NULL;
    g_value_array_append (array, &val);
    g_value_unset (&val);
  }

  GST_OBJECT_UNLOCK (obj);

  g_list_free (devices);

done:

  close (fd);

  return array;

/* ERRORS */
open_failed:
  {
    GST_OBJECT_UNLOCK (GST_OBJECT (probe));
    GST_WARNING_OBJECT (probe, "Can't open file descriptor to probe "
        "available devices: %s", g_strerror (errno));
    return NULL;
  }
legacy_oss:
  {
    close (fd);
    GST_OBJECT_UNLOCK (GST_OBJECT (probe));
    GST_DEBUG_OBJECT (probe, "Legacy OSS (ie. not OSSv4), not supported");
    return NULL;
  }
no_sysinfo:
  {
    close (fd);
    GST_OBJECT_UNLOCK (GST_OBJECT (probe));
    GST_WARNING_OBJECT (probe, "Can't open file descriptor to probe "
        "available devices: %s", g_strerror (errno));
    return NULL;
  }
}

#if 0
static void
gst_oss4_property_probe_interface_init (GstPropertyProbeInterface * iface)
{
  iface->get_properties = gst_oss4_property_probe_get_properties;
  iface->probe_property = gst_oss4_property_probe_probe_property;
  iface->needs_probe = gst_oss4_property_probe_needs_probe;
  iface->get_values = gst_oss4_property_probe_get_values;
}

void
gst_oss4_add_property_probe_interface (GType type)
{
  static const GInterfaceInfo probe_iface_info = {
    (GInterfaceInitFunc) gst_oss4_property_probe_interface_init,
    NULL,
    NULL,
  };

  g_type_add_interface_static (type, GST_TYPE_PROPERTY_PROBE,
      &probe_iface_info);
}
#endif
