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


#ifndef __GST_MULAWENCODE_H__
#define __GST_MULAWENCODE_H__

#include <gst/gst.h>
#include <gst/audio/gstaudioencoder.h>

G_BEGIN_DECLS
#define GST_TYPE_MULAWENC \
  (gst_mulawenc_get_type())
#define GST_MULAWENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MULAWENC,GstMuLawEnc))
#define GST_MULAWENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MULAWENC,GstMuLawEncClass))
#define GST_IS_MULAWENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MULAWENC))
#define GST_IS_MULAWENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MULAWENC))
typedef struct _GstMuLawEnc GstMuLawEnc;
typedef struct _GstMuLawEncClass GstMuLawEncClass;

struct _GstMuLawEnc
{
  GstAudioEncoder element;

  gint channels;
  gint rate;
};

struct _GstMuLawEncClass
{
  GstAudioEncoderClass parent_class;
};

GType gst_mulawenc_get_type (void);

G_END_DECLS
#endif /* __GST_STEREO_H__ */
