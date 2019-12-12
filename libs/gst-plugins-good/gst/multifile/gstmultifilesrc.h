/* GStreamer
 * Copyright (C) 2006 David A. Schleef <ds@schleef.org>
 *
 * gstmultifilesrc.c:
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

#ifndef __GST_MULTIFILESRC_H__
#define __GST_MULTIFILESRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS

#define GST_TYPE_MULTI_FILE_SRC \
  (gst_multi_file_src_get_type())
#define GST_MULTI_FILE_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MULTI_FILE_SRC,GstMultiFileSrc))
#define GST_MULTI_FILE_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MULTI_FILE_SRC,GstMultiFileSrcClass))
#define GST_IS_MULTI_FILE_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MULTI_FILE_SRC))
#define GST_IS_MULTI_FILE_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MULTI_FILE_SRC))

typedef struct _GstMultiFileSrc GstMultiFileSrc;
typedef struct _GstMultiFileSrcClass GstMultiFileSrcClass;

struct _GstMultiFileSrc
{
  GstPushSrc parent;

  gchar *filename;
  int start_index;
  int stop_index;
  int index;

  int offset;

  gboolean loop;

  GstCaps *caps;
  gboolean successful_read;

  gint fps_n, fps_d;
};

struct _GstMultiFileSrcClass
{
  GstPushSrcClass parent_class;
};

GType gst_multi_file_src_get_type (void);

G_END_DECLS

#endif /* __GST_MULTIFILESRC_H__ */
