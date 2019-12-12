/*-*- Mode: C; c-basic-offset: 2 -*-*/

/*
 *  GStreamer pulseaudio plugin
 *
 *  Copyright (c) 2004-2008 Lennart Poettering
 *
 *  gst-pulse is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  gst-pulse is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with gst-pulse; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 *  USA.
 */

#ifndef __GST_PULSESINK_H__
#define __GST_PULSESINK_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiosink.h>

#include <pulse/pulseaudio.h>
#include <pulse/thread-mainloop.h>

#include "pulseutil.h"

G_BEGIN_DECLS

#define GST_TYPE_PULSESINK \
  (gst_pulsesink_get_type())
#define GST_PULSESINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PULSESINK,GstPulseSink))
#define GST_PULSESINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PULSESINK,GstPulseSinkClass))
#define GST_IS_PULSESINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PULSESINK))
#define GST_IS_PULSESINK_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PULSESINK))
#define GST_PULSESINK_CAST(obj) \
  ((GstPulseSink *)(obj))

typedef struct _GstPulseSink GstPulseSink;
typedef struct _GstPulseSinkClass GstPulseSinkClass;

typedef struct _GstPulseDeviceInfo {
  gchar *description;
  GList *formats;
} GstPulseDeviceInfo;

struct _GstPulseSink
{
  GstAudioBaseSink sink;

  gchar *server, *device, *stream_name, *client_name;
  GstPulseDeviceInfo device_info;

  gdouble volume;
  gboolean volume_set:1;
  gboolean mute:1;
  gboolean mute_set:1;
  guint32 current_sink_idx;
  gchar *current_sink_name;

  guint defer_pending;

  gint notify; /* atomic */

  const gchar *pa_version;

  GstStructure *properties;
  pa_proplist *proplist;

  volatile gint format_lost;
  GstClockTime format_lost_time;
};

struct _GstPulseSinkClass
{
  GstAudioBaseSinkClass parent_class;
};

GType gst_pulsesink_get_type (void);

#define PULSE_SINK_TEMPLATE_CAPS \
  _PULSE_CAPS_PCM \
  _PULSE_CAPS_AC3 \
  _PULSE_CAPS_EAC3 \
  _PULSE_CAPS_DTS \
  _PULSE_CAPS_MP3 \
  _PULSE_CAPS_AAC

G_END_DECLS

#endif /* __GST_PULSESINK_H__ */
