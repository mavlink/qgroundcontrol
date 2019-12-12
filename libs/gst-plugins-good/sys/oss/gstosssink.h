/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *
 * gstosssink.h: 
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


#ifndef __GST_OSSSINK_H__
#define __GST_OSSSINK_H__


#include <gst/gst.h>
#include <gst/audio/gstaudiosink.h>

#include "gstosshelper.h"

G_BEGIN_DECLS

#define GST_TYPE_OSSSINK            (gst_oss_sink_get_type())
#define GST_OSSSINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OSSSINK,GstOssSink))
#define GST_OSSSINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OSSSINK,GstOssSinkClass))
#define GST_IS_OSSSINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OSSSINK))
#define GST_IS_OSSSINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_OSSSINK))

typedef struct _GstOssSink GstOssSink;
typedef struct _GstOssSinkClass GstOssSinkClass;

struct _GstOssSink {
  GstAudioSink    sink;

  gchar *device;
  gint   fd;
  gint   bytes_per_sample;

  GstCaps *probed_caps;
};

struct _GstOssSinkClass {
  GstAudioSinkClass parent_class;
};

GType gst_oss_sink_get_type(void);

G_END_DECLS

#endif /* __GST_OSSSINK_H__ */
