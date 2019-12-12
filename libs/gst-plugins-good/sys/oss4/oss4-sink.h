/* GStreamer OSS4 audio sink
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

#ifndef GST_OSS4_SINK_H
#define GST_OSS4_SINK_H


#include <gst/gst.h>
#include <gst/audio/gstaudiosink.h>

G_BEGIN_DECLS

#define GST_TYPE_OSS4_SINK            (gst_oss4_sink_get_type())
#define GST_OSS4_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OSS4_SINK,GstOss4Sink))
#define GST_OSS4_SINK_CAST(obj)       ((GstOss4Sink *)(obj))
#define GST_OSS4_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OSS4_SINK,GstOss4SinkClass))
#define GST_IS_OSS4_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OSS4_SINK))
#define GST_IS_OSS4_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_OSS4_SINK))

typedef struct _GstOss4Sink GstOss4Sink;
typedef struct _GstOss4SinkClass GstOss4SinkClass;

struct _GstOss4Sink {
  GstAudioSink  audio_sink;

  gchar       * device;             /* NULL if none was set      */
  gchar       * open_device;        /* the device we opened      */
  gchar       * device_name;        /* set if the device is open */
  gint          fd;                 /* -1 if not open            */
  gint          bytes_per_sample;
  gint          mute_volume;

  GstCaps     * probed_caps;
};

struct _GstOss4SinkClass {
  GstAudioSinkClass audio_sink_class;
};

GType  gst_oss4_sink_get_type (void);

G_END_DECLS

#endif /* GST_OSS4_SINK_H */


