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


#ifndef __GST_Y4MENCODE_H__
#define __GST_Y4MENCODE_H__


#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_Y4M_ENCODE \
  (gst_y4m_encode_get_type())
#define GST_Y4M_ENCODE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_Y4M_ENCODE, GstY4mEncode))
#define GST_Y4M_ENCODE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_Y4M_ENCODE, GstY4mEncodeClass))
#define GST_Y4M_ENCODE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_Y4M_ENCODE, GstY4mEncodeClass))
#define GST_IS_Y4M_ENCODE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_Y4M_ENCODE))
#define GST_IS_Y4M_ENCODE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_Y4M_ENCODE))

typedef struct _GstY4mEncode GstY4mEncode;
typedef struct _GstY4mEncodeClass GstY4mEncodeClass;

struct _GstY4mEncode {
  GstVideoEncoder parent;

  /* caps information */
  GstVideoInfo info;

  const gchar *colorspace;
  /* state information */
  gboolean header;
};

struct _GstY4mEncodeClass {
  GstVideoEncoderClass parent_class;
};

GType gst_y4m_encode_get_type(void);

G_END_DECLS

#endif /* __GST_Y4MENCODE_H__ */
