/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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


#ifndef __GST_STEREO_H__
#define __GST_STEREO_H__


#include <gst/gst.h>
#include <gst/audio/gstaudiofilter.h>

#define GST_TYPE_STEREO \
  (gst_stereo_get_type())
#define GST_STEREO(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_STEREO,GstStereo))
#define GST_STEREO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_STEREO,GstStereoClass))
#define GST_IS_STEREO(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_STEREO))
#define GST_IS_STEREO_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_STEREO))

typedef struct _GstStereo GstStereo;
typedef struct _GstStereoClass GstStereoClass;

struct _GstStereo {
  GstAudioFilter element;

  gboolean active;
  gfloat stereo;
};

struct _GstStereoClass {
  GstAudioFilterClass parent_class;
};

GType gst_stereo_get_type(void);

#endif /* __GST_STEREO_H__ */
