/* GStreamer OSS4 audio source
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

#ifndef GST_OSS4_SOURCE_H
#define GST_OSS4_SOURCE_H

#include <gst/gst.h>
#include <gst/audio/gstaudiosrc.h>

G_BEGIN_DECLS

#define GST_TYPE_OSS4_SOURCE            (gst_oss4_source_get_type())
#define GST_OSS4_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OSS4_SOURCE,GstOss4Source))
#define GST_OSS4_SOURCE_CAST(obj)       ((GstOss4Source *)(obj))
#define GST_OSS4_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OSS4_SOURCE,GstOss4SourceClass))
#define GST_IS_OSS4_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OSS4_SOURCE))
#define GST_IS_OSS4_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_OSS4_SOURCE))

typedef struct _GstOss4Source GstOss4Source;
typedef struct _GstOss4SourceClass GstOss4SourceClass;

struct _GstOss4Source {
  GstAudioSrc     audiosrc;

  gchar         * device;             /* NULL if none was set      */
  gchar         * open_device;        /* the device we opened      */
  gchar         * device_name;        /* set if the device is open */
  gint            fd;                 /* -1 if not open            */
  gint            bytes_per_sample;

  GstCaps       * probed_caps;
};

struct _GstOss4SourceClass {
  GstAudioSrcClass audiosrc_class;
};

GType  gst_oss4_source_get_type (void);

G_END_DECLS

#endif /* GST_OSS4_SOURCE_H */

