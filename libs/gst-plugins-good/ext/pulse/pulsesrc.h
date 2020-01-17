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

#ifndef __GST_PULSESRC_H__
#define __GST_PULSESRC_H__

#include <gst/gst.h>
#include <gst/audio/gstaudiosrc.h>

#include <pulse/pulseaudio.h>
#include <pulse/thread-mainloop.h>

G_BEGIN_DECLS

#define GST_TYPE_PULSESRC \
  (gst_pulsesrc_get_type())
#define GST_PULSESRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PULSESRC,GstPulseSrc))
#define GST_PULSESRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PULSESRC,GstPulseSrcClass))
#define GST_IS_PULSESRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PULSESRC))
#define GST_IS_PULSESRC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PULSESRC))
#define GST_PULSESRC_CAST(obj) \
  ((GstPulseSrc *)(obj))

typedef struct _GstPulseSrc GstPulseSrc;
typedef struct _GstPulseSrcClass GstPulseSrcClass;

struct _GstPulseSrc
{
  GstAudioSrc src;

  gchar *server, *device, *client_name;

  pa_threaded_mainloop *mainloop;

  pa_context *context;
  pa_stream *stream;
  guint32 source_output_idx;

  pa_sample_spec sample_spec;

  const void *read_buffer;
  size_t read_buffer_length;

  gchar *device_description;

  gdouble volume;
  gboolean volume_set:1;
  gboolean mute:1;
  gboolean mute_set:1;
  guint32 current_source_idx;
  gchar *current_source_name;

  gint notify; /* atomic */

  gboolean corked:1;
  gboolean stream_connected:1;
  gboolean operation_success:1;
  gboolean paused:1;
  gboolean in_read:1;

  GstStructure *properties;
  pa_proplist *proplist;
};

struct _GstPulseSrcClass
{
  GstAudioSrcClass parent_class;
};

GType gst_pulsesrc_get_type (void);

G_END_DECLS

#endif /* __GST_PULSESRC_H__ */
